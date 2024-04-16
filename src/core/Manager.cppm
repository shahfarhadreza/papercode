module;

#include <assert.h>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <memory>
#include <filesystem>
#include <yaml-cpp/yaml.h>

export module manager;

import project;

export enum class ProjectCreationError {
	None,
	Failed,
	FailedToSave,
	FailedToAddTemplateFile,
};

export struct Manager
{
	ProjectPtr mProject = nullptr;

	Manager() = default;
    ~Manager() = default;

    Manager(const Manager&) = delete;
    Manager& operator = (const Manager&) = delete;

    static Manager& get(){
        static Manager instance;
        return instance;
    }

    ProjectPtr getActiveProject() const { return mProject; }

    bool createProject(const ProjectTemplate& temp);
    void closeProject();
    bool openProject(const std::string& filepath);
    void saveProject();
};

bool Manager::createProject(const ProjectTemplate& temp) {
	const ProjectDesc& desc = temp.getDescription();

    mProject = std::make_shared<Project>();
    if (!mProject->createNew(desc)) {
        mProject = nullptr;
        std::cout << "Failed to create new project" << std::endl;
        return false;
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
        } else {
        	mProject = nullptr;
        	std::cout << "Failed to save the new project" << std::endl;
        	return false;
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
    return true;
}

void Manager::closeProject() {
	mProject = nullptr;
}

bool Manager::openProject(const std::string& filepath) {
	mProject = std::make_shared<Project>();
    if (!mProject->loadFromFile(filepath)) {
        mProject = nullptr;
        return false;
    }
    return true;
}

void Manager::saveProject() {
    if (mProject) {
        mProject->saveToFile();
    }
}



