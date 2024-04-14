#include "Stdafx.h"
#include "PaperCode.h"

void PaperCode::saveCurrentFile() {
    getUI().getEditorManager().saveActive();
}

void PaperCode::saveProject() {
    if (mProject) {
        mProject->saveToFile();
    }
}

void PaperCode::saveAll() {
    std::cout << "Saving everything..." << std::endl;
    // Save all the modified files
    getUI().getEditorManager().saveAll();
    // Save the project too
    saveProject();
}

void PaperCode::createProject(const ProjectTemplate& temp) {
    // Don't forget to close the current one (if has)
    closeProject();

    const ProjectDesc& desc = temp.getDescription();

    mProject = std::make_shared<Project>();
    if (!mProject->createNew(desc)) {
        mProject = nullptr;
        std::cout << "Failed to create new project" << std::endl;
        return;
    } else {

        // Add the template files to the project
        for (const ProjectTemplateFile& file : temp.getFiles()) {
            std::filesystem::path filePath = mProject->getDirectoryPath();
            filePath.append(file.getName());

            mProject->addFile(filePath.string());
        }
        // This will create the project directory (if doesn't exist already)
        // and create the actual project file (.paper)
        if (mProject->saveToFile()) {
            std::cout << "Successfuly created new project '" << mProject->getName() << "'" << std::endl;
        }
        // Create the template files and write template codes to them
        for (const ProjectTemplateFile& file : temp.getFiles()) {
            std::filesystem::path filePath = mProject->getDirectoryPath();
            filePath.append(file.getName());

            // Perhaps the project directory already contains a file with similar name?
            // Let's give a warning and skip creating file...
            if (std::filesystem::exists(filePath)) {
                // TODO: Warning Popup Dialog
                std::cout << "WARNING: Project directory already contains file '" << file.getName() << "'" << std::endl;
            } else {
                std::ofstream t(filePath);
                if (t.good()) {
                    t << file.getCode();
                    t.close();
                } else {
                    std::cout << "ERROR: Failed to create template file '" << filePath << "'" << std::endl;
                }
            }
            //printf("template file %s\n", filePath.c_str());
        }
    }

    mSettings.addToRecentProject(mProject->getFilePath().string());
    // TODO: Should we?
    openAllFiles();
}

void PaperCode::newProjectDialog() {
    getUI().mNewProject.open();
}

void PaperCode::newFileDialog() {
    getUI().newFileDialog(mProject);
}

void PaperCode::closeProject() {
    if (!mProject) {
        return;
    }
    if (mExecutionStatus != ExecutionStatus::None) {
        return;
    }
    getUI().getEditorManager().closeAllEditors();
    mProject = nullptr;
}

void PaperCode::openAllFiles() {
    if (!mProject) {
        return;
    }
    auto old = std::filesystem::current_path();
    std::filesystem::current_path(mProject->getDirectoryPath());

    // open all the files in editor
    for (ProjectFilePtr file : mProject->mFileList) {

        std::filesystem::path filePath(file->getPath());

        // If a project file has a relative path, we must provide an absolute path to the editor
        if (filePath.is_relative()) {
            //std::cout << "ERROR: Relative file path: '" << filePath << "'" << std::endl;
            //std::cout << "INFO: Absolute path: '" << std::filesystem::absolute(filePath) << "'" << std::endl;
            getUI().getEditorManager().openEditor(std::filesystem::absolute(filePath));
        } else {
            getUI().getEditorManager().openEditor(file->getPath());
        }
    }
    std::filesystem::current_path(old);
}

bool PaperCode::openProject(const std::string& filepath) {
    // Don't forget to close the current one (if has)
    closeProject();

    mProject = std::make_shared<Project>();
    if (!mProject->loadFromFile(filepath)) {
        mProject = nullptr;
        std::cout << "ERROR: Failed to load project" << std::endl;
        getUI().messageBox(std::format("Failed to load project '{}'", filepath), 
            UIMessageBoxType::Error, [this, &filepath](auto action) {
                mSettings.removeFromRecentProject(filepath);
            });
        return false;
    }
    mSettings.addToRecentProject(filepath);
    openAllFiles();
    return true;
}

void PaperCode::executeCommand(Commands cmd, CommandData data) {
    switch(cmd) {
    case Commands::NewProject:
        newProjectDialog();
        break;
    case Commands::NewFile:
        newFileDialog();
        break;
    case Commands::AddExistingFile:
        if (!data.FilePath.empty()) {
            if (getActiveProject()) {
                if (getActiveProject()->addFile(data.FilePath)) {
                    UISystem::get().notify(UINotification::ProjectFileAdded, {.FilePath = data.FilePath});
                }
            }
        }
        break;
    case Commands::RenameFile: 
        if (data.File) {
            ProjectFilePtr file = data.File;
            if (getActiveProject()) {
                ProjectPtr proj = getActiveProject();
                if (!data.FileNewName.empty()) {
                    std::string filePath = file->getAbsolutePath(proj).string();
                    if (file->rename(data.FileNewName, proj)) {
                        proj->saveToFile();
                        
                        UISystem::get().notify(UINotification::ProjectFileRenamed, 
                            {.FilePath = filePath, .FileNewPath = file->getAbsolutePath(proj).string()});
                    }
                }
            }
        }
        break;
    case Commands::Open:
        if (!data.FilePath.empty()) {
            openProject(data.FilePath);
        }
        break;
    case Commands::RemoveProjectFile:
        {
            ProjectFilePtr file = data.File;
            if (file) {
                std::string filePath = file->getPath();
                if (!getActiveProject()->removeFile(file)) {
                    UISystem::get().messageBox(std::format("Failed to remove file '{}' from project", 
                                                        file->getFileName()), UIMessageBoxType::Error);
                } else {
                    UISystem::get().notify(UINotification::ProjectFileRemoved, {.FilePath = filePath});
                }
            }
        }
        break;
    case Commands::Build:
        buildProject();
        break;
    case Commands::Run:
        runProject();
        break;
    case Commands::Stop:
        stopProject();
        break;
    case Commands::SaveCurrent:
        saveCurrentFile();
        break;
    case Commands::SaveAll:
        saveAll();
        break;
    case Commands::ProjectProperties:
        if (mProject) {
            getUI().mProjectProperties.open(mProject);
        }
        break;
    default:
        std::cout << "Unknown Command" << std::endl;
    }
}

bool PaperCode::init(const std::vector<std::string>& args) {

    if (!mSettings.deserialize()) {
        // First time run...
    }

    if (!getUI().init(768, 1324, "Paper Code")) {
        return false;
    }

    mCompiler.mName = "GNU GCC Compiler";
    mCompiler.mCompiler = "g++";
    mCompiler.mArchive = "ar"; // for building static library

    if (!args.empty()) {
        for(const auto& path: args) {
            getUI().getEditorManager().openEditor(path);
        }
    }

    return true;
}

bool PaperCode::run() {
    getUI().runEventLoop();
    return true;
}

bool PaperCode::terminate() {
    mExecutionStatus = ExecutionStatus::None;
    mSettings.serialize();
    closeProject();
    getUI().terminate();
    return true;
}

int main(int argc, char** argv) {
    PaperCode& app = PaperCode::get();
    std::vector<std::string> args;
    for(int i = 1;i < argc;++i) {
        args.push_back(argv[i]);
    }
    if (!app.init(args)) {
        assert(0);
        return -1;
    }
	if (!app.run()) {
        return -1;
	}
	app.terminate();
	return 0;
}

