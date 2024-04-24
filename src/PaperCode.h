#pragma once

import project;
import settings;
import subprocess;
import logsystem;
import buildoption;
import manager;

enum class FileContextMenuAction {
    None,
    Close,
    Save,
    Rename,
    Remove,
    Delete
};

// Platform Specific
struct UIExplorer {
    std::string mProjectNodeText = "";

    FileContextMenuAction drawTreeItem(const std::string& name, ProjectFilePtr file, ProjectPtr project);
    void draw();
};

struct UIElement {
    std::string mTitle;
};

class TextEditor;

struct UIEditor
{
protected:
    // Must be a full absolute path to the file
    std::filesystem::path mFilePath;
    std::string mFileName = "";
    bool mModified = false;
public:
    // The actual ImGui editor control
    std::shared_ptr<TextEditor> mImEditor = nullptr;
    bool mFirstLoaded = true;
    bool mFlagSelected = false;
public:
    UIEditor() { }
    ~UIEditor() { }

    void init();
    void destroy();

    int getLine() const;
    int getColumn() const;

    int getTabSize() const;

    const std::filesystem::path& getFilePath() const { return mFilePath; }
    const std::string& getFileName() const { return mFileName; }

    bool isFileOpen(const std::filesystem::path& filepath) const;

    void updateFilePath(const std::filesystem::path& filepath);

    void openFile(const std::filesystem::path& filepath);
    void saveToFile();

    bool isModified() const { return mModified; }
    void setModified(bool modified) {
        mModified = modified;
    }
};

using UIEditorPtr = std::shared_ptr<UIEditor>;
using UIEditorList = std::vector<UIEditorPtr>;

struct UIProjectProperties {

    bool mShow = false;
    std::shared_ptr<Project> mProject = nullptr;
    std::string mDirectory = "";
    std::string mObjDirectory = "";
    std::string mBinDirectory = "";
    ProjectDesc mProperties = {};

    void open(std::shared_ptr<Project> proj);
    void close();
    void applyChanges();
    void draw();
};

struct UIPreference {

    void open();
    void close();
    void applyChanges();

    void draw();
};

struct UINewProject {
    ProjectDesc mDesc;
    std::filesystem::path mDirectory;
    bool mAddHelloWorldTemplate = true;

    void open();
    void close();
    void create();

    void draw();
};

struct UINewFile {
    bool mTriggerPopup = false;
    ProjectPtr mProject = nullptr;
    std::string mName = "Untitled.cpp";
    std::filesystem::path mPath;

    void open(ProjectPtr project);
    void close();
    void create();

    void draw();
};

struct UIRenameFile {
    ProjectFilePtr mFile = nullptr;
    std::string mOriginalName = "";
    std::string mName = "";

    void open(ProjectFilePtr file);
    void close();
    void create();

    void draw();
};

enum class UIMessageBoxType {
    Information,
    YesNo,
    YesNoCancel,
    Error,
    Warning,
    Success,
};

enum class UIMessageBoxAction : int {
    Yes = 1,
    No = 2,
    Cancel = 3,
};

using UIMsgBoxFn = std::function<void(UIMessageBoxAction)>;

struct UIMessageBox {
    std::string mTitle = "";
    std::string mMsg = "";
    UIMessageBoxType mType = UIMessageBoxType::Information;
    UIMsgBoxFn mFunc = nullptr;

    void open(const std::string& title, const std::string& msg, UIMessageBoxType type, UIMsgBoxFn cb);
    void close();
    void draw();
};

enum class UINotification {
    None,
    ProjectFileAdded,
    ProjectFileRemoved,
    ProjectFileRenamed
};

struct UINotificationData {
    std::string FilePath = "";
    std::string FileNewPath = "";
};

struct UIEditorManager {
    UIEditorList mEditors;
    UIEditorPtr mActiveEditor = nullptr;
    bool mShowEditors = false;

    UIEditorPtr openEditor(const std::filesystem::path& filepath);
    void closeEditor(const std::string& filepath);
    void closeEditor(UIEditorPtr editor);
    void closeAllEditors();

    UIEditorPtr getEditor(const std::filesystem::path& filepath);

    UIEditorPtr getActiveEditor() { return mActiveEditor; }

    bool hasEditors() const { return !mEditors.empty(); }

    void saveActive();
    void saveAll();

    void draw();
};

struct UISystem {
    GLFWwindow* mWindow = nullptr;
    std::string mTitle = "";
    UIExplorer mExplorer;
    UIEditorManager mEditorManager;
    ImFont* mDefaultFontGUI = nullptr;
    ImFont* mDefaultMidFontGUI = nullptr;
    ImFont* mDefaultLargeFontGUI = nullptr;
    UIMessageBox mMsgBox;
    UIProjectProperties mProjectProperties;
    UINewProject mNewProject;
    UINewFile mNewFile;
    UIRenameFile mRenameFile;
    UIPreference mPreference;

    //
    bool mShowStatus = true;

    LogSystem mBuildLogs;

    UISystem() = default;
    ~UISystem() = default;

    UISystem(const UISystem&) = delete;
    UISystem& operator = (const UISystem&) = delete;

    static UISystem& get() {
        static UISystem instance;
        return instance;
    }

    GLFWwindow* getHandle() { return mWindow; }
    bool init(uint32_t height, uint32_t width, const char* title);
    void initFonts();

    std::string openFileDialog(std::vector<std::string> const &filters);
    void newFileDialog(ProjectPtr project);

    void notify(UINotification notif, UINotificationData data = {});

    void messageBox(const std::string& msg, UIMessageBoxType type, UIMsgBoxFn cb = nullptr);

    UIEditorManager& getEditorManager() { return mEditorManager; }

    void updateTitle(const std::string& title);

    void drawEvent();
    void drawUI();
    void drawUIStartPage();
    void drawToolbar();

    bool runEventLoop();
    void handleKeyboardInputs();
    void terminate();

    void clearBuildLogs();
    void appendBuildLog(const std::string& log, LogType type = LogType::General);
};

struct Compiler {
    std::string mName;
    std::string mDirectory;
    std::string mCompiler;
    std::string mArchive;
    std::string mOptions;
};

enum class Commands {
    NewProject,
    NewFile,
    AddExistingFile,
    Open,
    SaveCurrent,
    SaveAll,
    RenameFile,
    RemoveProjectFile,
    CompileCurrent,
    Build,
    Run,
    Stop,
    StartDebug,
    StopDebug,
    ProjectProperties,
    Settings
};

enum class ExecutionStatus {
    None,
    Building,
    Running
};

struct CommandData {
    ProjectPtr Project = nullptr;
    ProjectFilePtr File = nullptr;
    std::string FilePath = "";
    std::string FileNewName = "";
};

struct SmartSense;

/* Handles all the requests/commands/notifications between UI System and Manager
 */
struct PaperCode {
    ApplicationSettings mSettings;
    Compiler mCompiler;
    ExecutionStatus mExecutionStatus = ExecutionStatus::None;
    std::jthread mBuildThread;
    std::jthread mRunThread;
    std::shared_ptr<SubProcess> mChildProcess = nullptr;

    std::shared_ptr<SmartSense> mSmartSense = nullptr;

    PaperCode() = default;
    ~PaperCode() = default;

    PaperCode(const PaperCode&) = delete;
    PaperCode& operator = (const PaperCode&) = delete;

    static PaperCode& get(){
        static PaperCode instance;
        return instance;
    }

    std::shared_ptr<SmartSense> getSmartSense() { return mSmartSense; }
    UISystem& getUI() { return UISystem::get(); }
    Manager& getManager() { return Manager::get(); }
    ProjectPtr getActiveProject() { return getManager().getActiveProject(); }

    bool init(const std::vector<std::string>& args);
    bool run();
    bool terminate();

    void createProject(const ProjectTemplate& temp);
    bool openProject(const std::string& filepath);

    bool isBuilding() const;
    bool isProjectRunning() const;

    // Commands
    void newProjectDialog();
    void closeProject();
    void newFileDialog();

    void openAllFiles();

    void saveCurrentFile();
    void saveProject();
    void saveAll();

    void buildProject();
    void runProject();
    void stopProject();

    void updateProjectProperties(const ProjectDesc& desc);

    void executeCommand(Commands cmd, CommandData data = {});
};









