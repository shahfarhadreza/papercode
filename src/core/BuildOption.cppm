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

export module buildoption;

export enum class BuildSubSystem {
    Console,
    GUI,
    MAX,
};

export const char* BuildSubSystemString[] = {
    "Console",
    "GUI",
};

export enum class BuildLanguageStandard : int {
    CPP98,
    CPP11,
    CPP14,
    CPP17,
    CPP20,
    CPP23,
    MAX,
};

export const char* BuildLanguageStandardString[] = {
    "C++98",
    "C++11",
    "C++14",
    "C++17",
    "C++20",
    "C++23",
};

export enum class BuildType {
    Executable,
    StaticLibrary,
    DynamicLibrary,
    MAX,
};

export const char* BuildTypeString[] = {
    "Executable",
    "Static Library",
    "Dynamic Library",
};


export enum class BuildPlatform {
    Linux,
    Windows,
    MacOS,
    MAX,
};

export const char* BuildPlatformString[] = {
    "Linux",
    "Windows",
    "MacOS",
};

export struct BuildOption {
    BuildSubSystem mSubSystem = BuildSubSystem::Console;
    BuildLanguageStandard mLanguageStandard = BuildLanguageStandard::CPP11;
    BuildType mType = BuildType::Executable;

#if defined(WIN32)
    BuildPlatform mPlatform = BuildPlatform::Windows;
#elif defined (__unix__)
    BuildPlatform mPlatform = BuildPlatform::Linux;
#elif (defined (__APPLE__) && defined (__MACH__))
    BuildPlatform mPlatform = BuildPlatform::MacOS;
#else
    #error port to this platform
#endif

    std::string mAdditionalCompileFlags = "";
    std::string mAdditionalLinkFlags = "";

    //
    std::map<std::string, BuildLanguageStandard> langMap;
    std::map<std::string, BuildType> typeMap;
    std::map<std::string, BuildSubSystem> subSystemMap;

    BuildOption() {
        for (int i = 0;i < int(BuildLanguageStandard::MAX);i++) {
            std::string str = BuildLanguageStandardString[i];
            langMap[str] = BuildLanguageStandard(i);
        }
        for (int i = 0;i < int(BuildType::MAX);i++) {
            std::string str = BuildTypeString[i];
            typeMap[str] = BuildType(i);
        }
        for (int i = 0;i < int(BuildSubSystem::MAX);i++) {
            std::string str = BuildSubSystemString[i];
            subSystemMap[str] = BuildSubSystem(i);
        }
    }

    std::string getLanguageStandardAsString() const {
        return BuildLanguageStandardString[int(mLanguageStandard)];
    }

    void setLanguageStandardFromString(const std::string& str) {
        mLanguageStandard = langMap[str];
    }

    std::string getTypeAsString() const {
        return BuildTypeString[int(mType)];
    }

    void setTypeFromString(const std::string& str) {
        mType = typeMap[str];
    }

    std::string getSubSystemAsString() const {
        return BuildSubSystemString[int(mSubSystem)];
    }

    void setSubSystemFromString(const std::string& str) {
        mSubSystem = subSystemMap[str];
    }

    void serialize(YAML::Emitter& out) {
    	out << YAML::Key << "Language Standard" << YAML::Value << getLanguageStandardAsString();
    	out << YAML::Key << "Build Type" << YAML::Value << getTypeAsString();
        out << YAML::Key << "Sub System" << YAML::Value << getSubSystemAsString();
    	out << YAML::Key << "Compile Flags" << YAML::Value << mAdditionalCompileFlags;
    	out << YAML::Key << "Link Flags" << YAML::Value << mAdditionalLinkFlags;
    }

    void deserialize(const YAML::Node& data) {
    	if (data["Language Standard"]) {
	        std::string standard = data["Language Standard"].as<std::string>();
	        setLanguageStandardFromString(standard);
	    }
	    if (data["Build Type"]) {
	        std::string type = data["Build Type"].as<std::string>();
	        setTypeFromString(type);
	    }
        if (data["Sub System"]) {
            std::string ss = data["Sub System"].as<std::string>();
            setSubSystemFromString(ss);
        }
	    if (data["Compile Flags"]) {
	        mAdditionalCompileFlags = data["Compile Flags"].as<std::string>();
	    }
	    if (data["Link Flags"]) {
	        mAdditionalLinkFlags = data["Link Flags"].as<std::string>();
	    }
    }
};

