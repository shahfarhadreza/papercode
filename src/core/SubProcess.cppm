module;

/* This Module is heavily dependent on Windows Platform and Windows API functions
 */
#include <string>
#include <vector>
#include <iostream>
#include <functional>

#if defined(WIN32)

#include <windows.h>
#include <processthreadsapi.h>
#include <handleapi.h>

#elif defined (__unix__) || (defined (__APPLE__) && defined (__MACH__))

#include <unistd.h>

#else
    #error port to this platform
#endif

#define BUFSIZE 4096 

export module subprocess;

export enum SubProcessFlags : char {
    None = 1,
    CreateConsole = 2,
    RedirectOutput = 4,
};

export SubProcessFlags operator|(SubProcessFlags lhs, SubProcessFlags rhs)  {
    return static_cast<SubProcessFlags>(static_cast<char>(lhs) | static_cast<char>(rhs));
}

export SubProcessFlags operator&(SubProcessFlags lhs, SubProcessFlags rhs)  {
    return static_cast<SubProcessFlags>(static_cast<char>(lhs) & static_cast<char>(rhs));
}

export class ISubProcess
{
public:
    virtual ~ISubProcess() = default;

    virtual void cleanUp() = 0;
    virtual bool create(const std::string& cmd, SubProcessFlags flags) = 0;
    virtual void read(std::function<void(const std::string&)> cb) = 0;
    virtual void terminate() = 0;
    virtual bool isAlive() = 0;
    virtual bool getExitCode(int *pExitCode) = 0;
    virtual int wait() = 0;
};

#if defined(WIN32)

export class SubProcessWin32 : public ISubProcess
{
public:
    PROCESS_INFORMATION pi;

    HANDLE g_hChildStd_OUT_Rd = NULL;
    HANDLE g_hChildStd_OUT_Wr = NULL;

    SubProcessWin32() {
        ZeroMemory( &pi, sizeof(pi) );
        pi.hProcess = nullptr;
    }

    ~SubProcessWin32() {
        if (isAlive()) {
            terminate();
        }
        cleanUp();
    }

    void cleanUp() {
        if (pi.hProcess != nullptr) {
            // Close process and thread handles.
            CloseHandle( pi.hProcess );
        }
        pi.hProcess = nullptr;

        if (pi.hThread != nullptr) {
            CloseHandle(pi.hThread);
        }
        pi.hThread = nullptr;
    }

    bool create(const std::string& cmd, SubProcessFlags flags) {
        STARTUPINFOA siStartInfo;
        ZeroMemory( &siStartInfo, sizeof(siStartInfo) );

        SECURITY_ATTRIBUTES saAttr;

        // Set the bInheritHandle flag so pipe handles are inherited.
        saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
        saAttr.bInheritHandle = TRUE;
        saAttr.lpSecurityDescriptor = NULL;

        bool redirectOutput = flags & SubProcessFlags::RedirectOutput;

        if (redirectOutput) {
            // Create a pipe for the child process's STDOUT. 
            if ( ! CreatePipe(&g_hChildStd_OUT_Rd, &g_hChildStd_OUT_Wr, &saAttr, 0) ) {
                printf("CRITICAL ERROR: Failed to create pipe for STDOUT\n");
                return false;
            }
            // Ensure the read handle to the pipe for STDOUT is not inherited
            if ( ! SetHandleInformation(g_hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0) ){
                printf("CRITICAL ERROR: STDOUT is inherited\n");
                return false;
            }
        }

        siStartInfo.cb = sizeof(siStartInfo);

        siStartInfo.wShowWindow = flags & SubProcessFlags::CreateConsole ? SW_SHOW : SW_HIDE;

        siStartInfo.dwFlags = STARTF_USESHOWWINDOW;

        if(redirectOutput) {
            siStartInfo.dwFlags |= STARTF_USESTDHANDLES;
            siStartInfo.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
            siStartInfo.hStdOutput = g_hChildStd_OUT_Wr;
            siStartInfo.hStdError = g_hChildStd_OUT_Wr;
        }

        DWORD creationFlags = flags & SubProcessFlags::CreateConsole ? CREATE_NEW_CONSOLE : CREATE_NO_WINDOW;

        std::string argv = cmd;

        if( !CreateProcessA(nullptr,   // No module name (use command line)
            &argv[0],        // Command line
            nullptr,           // Process handle not inheritable
            nullptr,           // Thread handle not inheritable
            true,               // handles are inherited
            creationFlags,      // creation flags
            nullptr,           // Use parent's environment block
            nullptr,           // Use parent's starting directory
            &siStartInfo,                // Pointer to STARTUPINFO structure
            &pi )               // Pointer to PROCESS_INFORMATION structure
        )
        {
            return false;
        }

        if (redirectOutput) {
            CloseHandle(g_hChildStd_OUT_Wr);
        }
        return true;
    }

    void read(std::function<void(const std::string&)> cb) {
        DWORD dwRead; 
        CHAR chBuf[BUFSIZE]; 
        BOOL bSuccess = FALSE;

        for (;;)  {
            bSuccess = ReadFile(g_hChildStd_OUT_Rd, chBuf, BUFSIZE, &dwRead, NULL);
            if( ! bSuccess || dwRead == 0 ) 
                break; 

            std::string ss(chBuf, dwRead);
            cb(ss);
        } 
    }

    void terminate() {
        if (pi.hProcess == nullptr) {
            return;
        }
        TerminateProcess(pi.hProcess, 0);
    }

    bool isAlive() {
        if (pi.hProcess == nullptr) {
            return false;
        }
        int exitCode;
        getExitCode(&exitCode);
        return exitCode == STILL_ACTIVE;
    }

    bool getExitCode(int *pExitCode) {
        if (pi.hProcess == nullptr) {
            return false;
        }
        DWORD exitCode;
        BOOL ret = GetExitCodeProcess(pi.hProcess, &exitCode);
        *pExitCode = exitCode;
        return ret;
    }

    int wait() {
        return (int) WaitForSingleObject(pi.hProcess, INFINITE); 
    }
};

export using SubProcess = SubProcessWin32;

#elif defined (__unix__) || (defined (__APPLE__) && defined (__MACH__))

#include <cstring>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

class SubProcessPosix : public ISubProcess
{
public:
    SubProcessPosix()
        : pid(-1)
        , running(false)
        , exitCode(-1)
    {
        // Initialize pipes
        if (pipe(outputPipe) == -1) {
            std::cerr << "Failed to create output pipe" << std::endl;
        }
    }

    void cleanUp() override {
        // Clean up any resources if needed
    }

    bool create(const std::string &cmd, SubProcessFlags flags) override {
        std::vector<char *> args;
        tokenize(cmd, args);
        args.push_back(nullptr);

        pid = fork();
        if (pid == 0) {
            // Child process
            if (flags & SubProcessFlags::RedirectOutput) {
                // Redirect stdout to the write end of the pipe
                dup2(outputPipe[1], STDOUT_FILENO);
            }
            close(outputPipe[0]); // Close the read end of the pipe

            if (execvp(args[0], args.data()) == -1) {
                std::cerr << "Failed to execute command: " << cmd << std::endl;
                exit(EXIT_FAILURE);
            }
        } else if (pid > 0) {
            // Parent process
            running = true;
            close(outputPipe[1]); // Close the write end of the pipe
        } else {
            // Fork failed
            std::cerr << "Fork failed" << std::endl;
            return false;
        }
        return true;
    }

    void read(std::function<void(const std::string &)> cb) override {
        if (!running) {
            std::cerr << "Process is not running" << std::endl;
            return;
        }

        char buffer[BUFSIZ];
        ssize_t bytesRead;
        while ((bytesRead = ::read(outputPipe[0], buffer, BUFSIZ)) > 0) {
            std::string data(buffer, bytesRead);
            cb(data);
        }
        if (bytesRead == -1) {
            std::cerr << "Failed to read from pipe" << std::endl;
        }
    }

    void terminate() override {
        if (running) {
            kill(pid, SIGTERM);
            running = false;
        }
    }

    bool isAlive() override {
        if (!running) {
            return false;
        }

        int status;
        if (waitpid(pid, &status, WNOHANG) == 0) {
            return true;
        } else {
            running = false;
            return false;
        }
    }

    bool getExitCode(int *pExitCode) override {
        if (isAlive()) {
            return false;
        }
        if (exitCode != -1) {
            *pExitCode = exitCode;
            return true;
        }
        return false;
    }

    int wait() override {
        if (running) {
            int status;
            if (waitpid(pid, &status, 0) > 0) {
                if (WIFEXITED(status)) {
                    exitCode = WEXITSTATUS(status);
                    return exitCode;
                }
            }
        }
        return -1;
    }

private:
    pid_t pid;
    bool running;
    int exitCode;
    int outputPipe[2]; // Pipe for reading subprocess output

    void tokenize(const std::string &cmd, std::vector<char *> &args) {
        // Simple tokenization function for demonstration
        std::string token;
        for (char c : cmd) {
            if (c == ' ') {
                if (!token.empty()) {
                    args.push_back(strdup(token.c_str()));
                    token.clear();
                }
            } else {
                token += c;
            }
        }
        if (!token.empty()) {
            args.push_back(strdup(token.c_str()));
        }
    }
};
export using SubProcess = SubProcessPosix;

#else
    #error port to this platform
#endif

