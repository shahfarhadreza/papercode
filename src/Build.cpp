#include "Stdafx.h"
#include "PaperCode.h"

#include <chrono>
using namespace::std::literals;

bool PaperCode::isProjectRunning() const {
    return mChildProcess && mChildProcess->isAlive();//mExecutionStatus == ExecutionStatus::Running;
}

void PaperCode::stopProject() {
    if (mChildProcess) {
        mChildProcess->terminate();
    }
}

void PaperCode::runProject() {

    if (isProjectRunning()) {
        return;
    }

    ProjectPtr mProject = getActiveProject();

    if (!mProject) {
        return;
    }

    mChildProcess = std::make_shared<SubProcess>();

    mRunThread = std::jthread([this, mProject] () {

        UISystem& mUISystem = getUI();

        mExecutionStatus = ExecutionStatus::Running;

        const BuildOption& buildOption = mProject->mDesc.mBuildOption;

        std::string exeFile = mProject->getBinWithFullPath().string();

        std::string cmd = std::format("{}", exeFile);

        bool pauseAfterExecution = true;

        if (buildOption.mSubSystem != BuildSubSystem::GUI) {
            if (pauseAfterExecution) {
                cmd = std::format(".\\console-exec.exe {}", exeFile);
            }
        }

        mUISystem.clearBuildLogs();
        mUISystem.appendBuildLog("Executing: " + cmd + "\r\n");

        if (!mChildProcess->create(cmd, SubProcessFlags::CreateConsole)) {
            mUISystem.appendBuildLog(std::format("Failed to execute command '{}'\r\n", cmd), LogType::Error);
        } else {
/*
            const DWORD result = mChildProcess->wait();
            if (result != WAIT_OBJECT_0) {
                // Timed out or an error occurred
                mUISystem.appendBuildLog("Failed to wait for process to finish\r\n", LogType::Error);
            }
*/

            while(1) {
                int exitCode;
                if (mChildProcess->getExitCode(&exitCode)) {
                    if (!mChildProcess->isAlive()) {
                        mUISystem.appendBuildLog(std::format("Process terminated with status {} (0 seconds(s))\r\n", exitCode), LogType::Info);
                        break;
                    }
                } else {
                    mUISystem.appendBuildLog(std::format("Failed to get exit code\r\n"), LogType::Error);
                    break;
                }
            }

            mChildProcess->terminate();
        }

        mExecutionStatus = ExecutionStatus::None;
    });
}

bool PaperCode::isBuilding() const {
    return mExecutionStatus == ExecutionStatus::Building;
}

bool CompileProjectFiles(ProjectPtr proj) {
    return true;
}

void PaperCode::buildProject() {

    ProjectPtr mProject = getActiveProject();

    // No project to build?
    if (!mProject) {
        return;
    }

    // Already running?
    if (isBuilding()) {
        return;
    }

    // Hit the save
    saveAll();

    // Check for compiler
    {
        auto compiler = std::make_shared<SubProcess>();

        if (!compiler->create("g++ -v", SubProcessFlags::None)) {
            getUI().messageBox("Failed to execute command 'g++ -v'. GCC compiler missing?", UIMessageBoxType::Error);
            return;
        }
    }

    // Make sure we have all the directories we need to put our compiled files into

    // Project directory is our current working directory
    auto old = std::filesystem::current_path();
    std::filesystem::path projectPath(mProject->getDirectoryPath());
    std::filesystem::current_path(projectPath);

    // Check for relative paths
    std::filesystem::path objAbsolutePath(mProject->getObjPath());

    std::cout << "LOG: Fixing object relative path..." << std::endl;
    if (objAbsolutePath.is_relative()) {
        objAbsolutePath = std::filesystem::absolute(objAbsolutePath);
    }


    bool objPathExists = std::filesystem::is_directory(objAbsolutePath);

    if (!objPathExists) {
        if (!std::filesystem::create_directories(objAbsolutePath)) {
            getUI().appendBuildLog("Failed to create directory for object files: '" + objAbsolutePath.string() + "'\r\n", LogType::Error);
            getUI().appendBuildLog("Build stopped.\r\n");
            return;
        }
    }

    // Check for relative paths
    std::filesystem::path binAbsolutePath(mProject->getBinPath());

    std::cout << "LOG: Fixing output relative path..." << std::endl;
    if (binAbsolutePath.is_relative()) {
        binAbsolutePath = std::filesystem::absolute(binAbsolutePath);
    }

    bool binPathExists = std::filesystem::is_directory(binAbsolutePath);

    if (!binPathExists) {
        if (!std::filesystem::create_directories(binAbsolutePath)) {
            getUI().appendBuildLog("Failed to create directory for output/bin file: '" + binAbsolutePath.string() + "'\r\n", LogType::Error);
            getUI().appendBuildLog("Build stopped.\r\n");
            return;
        }
    }

    std::filesystem::current_path(old);

    std::cout << "LOG: Starting build thread..." << std::endl;
    mBuildThread = std::jthread([this, mProject, objAbsolutePath, binAbsolutePath] () {

        UISystem& mUISystem = getUI();

        mExecutionStatus = ExecutionStatus::Building;

        mUISystem.clearBuildLogs();
        mUISystem.appendBuildLog("\r\n");
        mUISystem.appendBuildLog("-------------- Build: " + mProject->getName() + " (compiler: " + mCompiler.mName + ")---------------\r\n", LogType::Info);
        mUISystem.appendBuildLog("\r\n");

        //CompileProjectFiles(mProject);
        const BuildOption& buildOption = mProject->mDesc.mBuildOption;

        bool compileSuccess = true;

        bool debugBuild = true;

        for (ProjectFilePtr file : mProject->mFileList) {

            if (!file->isCompile()) {
                continue;
            }
            std::filesystem::path objFile = objAbsolutePath;
            objFile.append(file->getFileNameNoExt() + ".o");

            std::string compilerOptions = "-Wall -fexceptions ";

            if (debugBuild) {
                compilerOptions += "-g ";
            }

            switch(buildOption.mLanguageStandard) {
            case BuildLanguageStandard::CPP98:
                compilerOptions += "-std=c++98 ";
                break;
            case BuildLanguageStandard::CPP11:
                compilerOptions += "-std=c++11 ";
                break;
            case BuildLanguageStandard::CPP14:
                compilerOptions += "-std=c++14 ";
                break;
            case BuildLanguageStandard::CPP17:
                compilerOptions += "-std=c++17 ";
                break;
            case BuildLanguageStandard::CPP20:
                compilerOptions += "-std=c++20 ";
                break;
            case BuildLanguageStandard::CPP23:
                compilerOptions += "-std=c++23 ";
                break;
            default:
                compilerOptions += "-std=c++11 ";
                break;
            }

            compilerOptions += buildOption.mAdditionalCompileFlags + " ";

            std::string cmd = mCompiler.mCompiler + " " + 
                compilerOptions + " " + 
                " -c " + file->getAbsolutePath(mProject).string() +
                " -o " + objFile.string();

            mUISystem.appendBuildLog(cmd);
            mUISystem.appendBuildLog("\r\n\r\n");

            mChildProcess = std::make_shared<SubProcess>();

            if (!mChildProcess->create(cmd, SubProcessFlags::RedirectOutput)) {
                mUISystem.appendBuildLog(std::format("Failed to execute command '{}'\r\n", cmd), LogType::Error);
                continue;
            }

            mChildProcess->read([this](const std::string& line) {
                 getUI().appendBuildLog(line);
            });

            while(1) {
                int exitCode;
                if (mChildProcess->getExitCode(&exitCode)) {
                    if (!mChildProcess->isAlive()) {
                        if (exitCode != 0) {
                            compileSuccess = false;
                        }
                        mUISystem.appendBuildLog(std::format("Process terminated with status {} (0 seconds(s))\r\n", exitCode), LogType::Info);
                        break;
                    }
                } else {
                    mUISystem.appendBuildLog(std::format("Failed to get exit code\r\n"), LogType::Error);
                    break;
                }
            }

            mChildProcess->terminate();
        }

        if (compileSuccess) {
            mUISystem.appendBuildLog("Linking...\r\n");

            bool buildSuccess = true;
            std::string outputFile = mProject->getBinWithFullPath().string();

            std::string cmd = std::format("{} {} -o {} ", mCompiler.mCompiler, buildOption.mAdditionalLinkFlags, outputFile);

            if (buildOption.mType == BuildType::StaticLibrary) {
                cmd = std::format("{} {} crf {} ", mCompiler.mArchive, buildOption.mAdditionalLinkFlags, outputFile);
            }

            for (ProjectFilePtr file : mProject->mFileList) {
                // If not compiled, then not linked
                if (!file->isCompile()) {
                    continue;
                }
                std::filesystem::path objFile = objAbsolutePath;
                objFile.append(file->getFileNameNoExt() + ".o");

                cmd += objFile.string() + " ";
            }

            if (buildOption.mSubSystem == BuildSubSystem::GUI) {
                cmd += "-Wl,--subsystem,windows ";
            }

            mUISystem.appendBuildLog(cmd);
            mUISystem.appendBuildLog("\r\n");

            // Reinitialize the process object for the linking step-
            // TODO: There should be a better/proper way?
            mChildProcess = std::make_shared<SubProcess>();

            if (mChildProcess->create(cmd, SubProcessFlags::RedirectOutput)) {
                mChildProcess->read([this](const std::string& line) {
                     getUI().appendBuildLog(line);
                });

                while(1) {
                    int exitCode;
                    if (mChildProcess->getExitCode(&exitCode)) {
                        if (!mChildProcess->isAlive()) {
                            if (exitCode != 0) {
                                buildSuccess = false;
                            }
                            mUISystem.appendBuildLog(std::format("Process terminated with status {} (0 seconds(s))\r\n", exitCode), LogType::Info);
                            break;
                        }
                    } else {
                        mUISystem.appendBuildLog(std::format("Failed to get exit code\r\n"), LogType::Error);
                        break;
                    }
                }

                mChildProcess->terminate();

                if (buildSuccess) {
                    mUISystem.appendBuildLog("Build successfuly.\r\n", LogType::Success);
                }
            } else {
                mUISystem.appendBuildLog(std::format("Failed to execute command '{}'\r\n", cmd), LogType::Error);
            }
        } else {
            mUISystem.appendBuildLog("Compiled with error(s).\r\n", LogType::Error);
        }

        mExecutionStatus = ExecutionStatus::None;
    });
}


