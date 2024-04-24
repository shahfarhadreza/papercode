module;

#include <assert.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>
#include <yaml-cpp/yaml.h>

export module settings; 

export struct Recent {
    std::string filepath;
    std::string nameonly;

    bool operator==(const Recent& rhs) const { return this->filepath == rhs.filepath;}
};

export struct ApplicationSettings {
    std::string mFileName = "settings.dat";
    std::vector<Recent> mRecentProjects;

    float mEditorFontSize = 15.0f;
    std::string mThemeName = "Default";

    void addToRecentProject(const std::string& filepath);
    void removeFromRecentProject(const std::string& filepath);
    void removeRecent(const Recent& recent);

    void serialize();
    bool deserialize();
};

void ApplicationSettings::addToRecentProject(const std::string& filepath) {

    std::string fileNameOnly = std::filesystem::path(filepath).stem().string();

    Recent rc;

    rc.filepath = filepath;
    rc.nameonly = fileNameOnly;

    auto it = std::find(mRecentProjects.cbegin(), mRecentProjects.cend(), rc);

    if (it == mRecentProjects.end()) {
        mRecentProjects.insert(mRecentProjects.begin(), rc);
    }
}

void ApplicationSettings::removeFromRecentProject(const std::string& filepath) {
    std::string fileNameOnly = std::filesystem::path(filepath).stem().string();

    Recent rc;

    rc.filepath = filepath;
    rc.nameonly = fileNameOnly;

    removeRecent(rc);
}

void ApplicationSettings::removeRecent(const Recent& recent) {
    auto it = std::find(mRecentProjects.cbegin(), mRecentProjects.cend(), recent);

    if (it != mRecentProjects.end()) {
        mRecentProjects.erase(it);
    }
}

void ApplicationSettings::serialize() {

    YAML::Emitter out;

    out << YAML::BeginMap;

    out << YAML::Key << "Editor Font Size" << YAML::Value << mEditorFontSize;

    out << YAML::Key << "Recent Projects" << YAML::Value << YAML::BeginSeq;

    for(const Recent& rc : mRecentProjects) {
        out << YAML::BeginMap;
        out << YAML::Key << "Name" << YAML::Value << rc.nameonly;
        out << YAML::Key << "Path" << YAML::Value << rc.filepath;
        out << YAML::EndMap;
    }

    out << YAML::EndSeq;

    out << YAML::EndMap;

    std::ofstream ofile(mFileName);

    ofile << out.c_str();
    ofile.close();
}

bool ApplicationSettings::deserialize() {

    if (!std::filesystem::exists(mFileName)) {
        // First time running
        return false;
    }

    YAML::Node data;
    try {
        data = YAML::LoadFile(mFileName);
    }
    catch (YAML::ParserException e) {
        return false;
    }

    mEditorFontSize = data["Editor Font Size"].as<float>();

    auto files = data["Recent Projects"];
    if (files) {
        for (auto file : files) {
            Recent rc;
            rc.nameonly = file["Name"].as<std::string>();
            rc.filepath = file["Path"].as<std::string>();

            mRecentProjects.push_back(rc);
        }
    }
    return true;
}

