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

export module project;

import buildoption;

export class Project;
export class ProjectFile;

export using ProjectPtr = std::shared_ptr<Project>;

export using ProjectFilePtr = std::shared_ptr<ProjectFile>;
export using ProjectFileList = std::vector<ProjectFilePtr>;

export class ProjectFile
{
public:
    std::string mFullPath; // Can be relative or absolute
    std::string mFileName;
    std::string mFileNameOnly;
    bool mCompile = true;
public:
    ProjectFile(const std::string& path);

    bool rename(const std::string& newName, ProjectPtr project);

    // Returns the absolute path of the file relative to given project's path
    std::filesystem::path getAbsolutePath(ProjectPtr project) const;

    const std::string& getPath() const { return mFullPath; }
    const std::string& getFileName() const { return mFileName; }
    const std::string& getFileNameNoExt() const { return mFileNameOnly; }

    bool isCompile() const { return mCompile; }
};

export struct ProjectDesc {
    std::string mName;
    std::filesystem::path mFilePath;
    std::string mObjPath; 
    std::string mBinPath;
    BuildOption mBuildOption;
};

export struct ProjectTemplateFile {
    std::string mName; // File name with extension (.cpp/.h etc.)
    std::string mCode; // The template code

    const auto& getName() const { return mName; }
    const auto& getCode() const { return mCode; }
};

export struct ProjectTemplate {
    ProjectDesc mDesc;
    std::vector<ProjectTemplateFile> mFiles;

    void addTemplateFile(const std::string& filename, const std::string& code) {
        mFiles.push_back({.mName = filename, .mCode = code});
    }

    const ProjectDesc& getDescription() const { return mDesc; }
    const auto& getFiles() const { return mFiles; }
};

export class Project
{
public:
    ProjectDesc mDesc;
    ProjectFileList mFileList;
public:
    Project();
    ~Project();

    bool createNew(const ProjectDesc& desc);
    bool loadFromFile(const std::filesystem::path& filepath);
    bool saveToFile();

    ProjectFilePtr addNewFile(const std::string& path);
    ProjectFilePtr addFile(const std::string& file);

    bool removeFile(ProjectFilePtr file);

    const std::string& getName() const { return mDesc.mName; }
    void setName(const std::string& name);

    const std::filesystem::path& getFilePath() const { return mDesc.mFilePath; }
    std::filesystem::path getDirectoryPath() const { return mDesc.mFilePath.parent_path(); }
    const std::string& getObjPath() const { return mDesc.mObjPath; }
    const std::string& getBinPath() const { return mDesc.mBinPath; }

    std::filesystem::path getBinWithFullPath() const;

    const BuildOption& getBuildOption() const { return mDesc.mBuildOption; }
};

ProjectFile::ProjectFile(const std::string& path)
    : mFullPath(path) {
    mFileName = std::filesystem::path(mFullPath).filename().string();
    mFileNameOnly = std::filesystem::path(mFullPath).stem().string();

    std::string ext = std::filesystem::path(mFullPath).extension().string();

    std::transform(ext.begin(), ext.end(), ext.begin(), 
        [](unsigned char c){ return std::tolower(c); });

    if (ext == ".cpp" || ext == ".cxx" || ext == ".cc" || ext == ".c") {
        mCompile = true;
    } else {
        mCompile = false; 
    }
}

bool ProjectFile::rename(const std::string& newName, ProjectPtr project) {

    std::filesystem::path newFilePath = std::filesystem::path(mFullPath).parent_path();

    if (std::filesystem::path(mFullPath).is_relative()) {
        auto old = std::filesystem::current_path();
        std::filesystem::current_path(project->getDirectoryPath());

        newFilePath.append(newName);
        std::filesystem::rename(mFullPath, newFilePath);

        std::filesystem::current_path(old);
    } else {
        newFilePath.append(newName);
        std::filesystem::rename(mFullPath, newFilePath);
    }

    mFullPath = newFilePath.string();
    mFileName = newFilePath.filename().string();
    mFileNameOnly = newFilePath.stem().string();

    return true;
}

std::filesystem::path ProjectFile::getAbsolutePath(ProjectPtr project) const {
    std::filesystem::path newPath(mFullPath);

    if (newPath.is_relative()) {

        auto old = std::filesystem::current_path();
        std::filesystem::current_path(project->getDirectoryPath());

        newPath = std::filesystem::absolute(newPath);

        std::filesystem::current_path(old);
    }

    return newPath;
}

Project::Project() {

}

Project::~Project() {
    mFileList.clear();
}

std::filesystem::path Project::getBinWithFullPath() const {
    std::filesystem::path binAbsolutePath(getBinPath());
    if (binAbsolutePath.is_relative()) {
        auto old = std::filesystem::current_path();
        std::filesystem::path projectPath(getDirectoryPath());
        std::filesystem::current_path(projectPath);

        binAbsolutePath = std::filesystem::absolute(binAbsolutePath);

        std::filesystem::current_path(old);
    }
    if (mDesc.mBuildOption.mType == BuildType::Executable) {
        switch(mDesc.mBuildOption.mPlatform) {
        case BuildPlatform::Windows:
            binAbsolutePath.append(getName() + ".exe");
            break;
        case BuildPlatform::MacOS:
            binAbsolutePath.append(getName() + ".app");
            break;
        default:
            binAbsolutePath.append(getName());
            break;
        }
    } else {
        binAbsolutePath.append(std::format("lib{}.a", getName()));
    }
    return binAbsolutePath;
}

bool Project::createNew(const ProjectDesc& desc) {
    mDesc = desc;
    return true;
}

bool Project::loadFromFile(const std::filesystem::path& file) {

    // Check if the file actually exist
    if (!std::filesystem::exists(file)) {
        std::cout << "ERROR: Project file '" << file << "' doesn't exist" << std::endl;
        return false;
    }

    std::filesystem::path pathOnly = file.parent_path();
    std::cout << "LOG: Project directory path '" << pathOnly << "'" << std::endl;

    mDesc.mFilePath = file;

    YAML::Node data;
    try {
        data = YAML::LoadFile(file.string());
    }
    catch (YAML::ParserException e) {
        assert(0);
        return false;
    }
    mDesc.mName = data["Project"].as<std::string>();
    mDesc.mObjPath = data["Objects Directory"].as<std::string>();
    mDesc.mBinPath = data["Binary Directory"].as<std::string>();

    mDesc.mBuildOption.deserialize(data);

    auto files = data["Files"];
    if (files) {
        for (auto file : files) {
            std::string path = file["Path"].as<std::string>();
            addFile(path);
        }
    }
    return true;
}

bool Project::saveToFile() {
    YAML::Emitter out;
    const std::string projPath = getDirectoryPath().string();

    if (getName().empty() || projPath.empty()) {
        std::cout << "ERROR: Empty project name/path. Failed to save." << std::endl;
        return false;
    }

    std::cout << "LOG: Saving project at '" << projPath << "'..." << std::endl;

    // See if the project directory exist/create otherwise
    bool projPathExists = std::filesystem::is_directory(projPath);

    if (!projPathExists) {
        std::cout << "LOG: Project directory doesn't exist, creating one..." << std::endl;
        if (!std::filesystem::create_directories(projPath)) {
            std::cout << "CRITICAL ERROR: Failed to create project directory '" << projPath << "'" << std::endl;
            return false;
        } else {
            std::cout << "LOG: Project directory created." << std::endl;
        }
    }
    out << YAML::BeginMap;

    out << YAML::Key << "Project" << YAML::Value << getName();
    out << YAML::Key << "Objects Directory" << YAML::Value << getObjPath();
    out << YAML::Key << "Binary Directory" << YAML::Value << getBinPath();

    mDesc.mBuildOption.serialize(out);

    out << YAML::Key << "Files" << YAML::Value << YAML::BeginSeq;

    for(ProjectFilePtr file : mFileList) {
        out << YAML::BeginMap;
        out << YAML::Key << "Path" << YAML::Value << file->getPath();
        out << YAML::EndMap;
    }

    out << YAML::EndSeq;

    out << YAML::EndMap;

    std::ofstream ofile(getFilePath());

    ofile << out.c_str();
    ofile.close();

    std::cout << "LOG: Project saved" << std::endl;
    return true;
}

void Project::setName(const std::string& name) {
    mDesc.mName = name;
}

ProjectFilePtr Project::addNewFile(const std::string& path) {
    // Then add the file to project
    ProjectFilePtr newFile = std::make_shared<ProjectFile>(path);
    mFileList.push_back(newFile);
    return newFile;
}

ProjectFilePtr Project::addFile(const std::string& path) {
    ProjectFilePtr newFile = std::make_shared<ProjectFile>(path);
    mFileList.push_back(newFile);
    return newFile;
}

bool Project::removeFile(ProjectFilePtr file) {
    auto it = std::find(mFileList.begin(), mFileList.end(), file);
    if (it != mFileList.end()) {
        mFileList.erase(it);
        return true;
    }
    return false;
}






