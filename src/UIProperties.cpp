#include "Stdafx.h"
#include "PaperCode.h"
#include "ImGuiHelper.h"

void UIRenameFile::open(ProjectFilePtr file) {
    mName = file->getFileName();
    mOriginalName = mName;
    mFile = file;
    ImGui::OpenPopup("Rename File");
}

void UIRenameFile::close() {
    ImGui::CloseCurrentPopup();
}

void UIRenameFile::create() {
    if (!mName.empty()) {
        if (mName != mOriginalName) {
            PaperCode::get().executeCommand(Commands::RenameFile, {.File = mFile, .FileNewName = mName});
        }
    }
}

void UIRenameFile::draw() {
    ImGuiStyle& style = ImGui::GetStyle();

    ImGui_DrawProperties("Name:", &mName, 0, false, false);

    float width = ImGui::GetWindowWidth();
    float height = ImGui::GetWindowHeight();

    ImGui::SetCursorPosX(width - 190);
    ImGui::SetCursorPosY(height - 45);

    if (ImGui::Button("Ok", ImVec2(80, 25))) {
        create();
        close();
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel", ImVec2(80, 25))) {
        close();
    }
}

void UINewFile::open(ProjectPtr project) {
    mProject = project;
    mName = "Untitles.cpp";
    mPath = ".\\";
    ImGui::OpenPopup("New File");
}

void UINewFile::close() {
    ImGui::CloseCurrentPopup();
}

void UINewFile::create() {
    std::filesystem::path filePath = mPath;
    filePath.append(mName);
    mProject->addNewFile(filePath.string());
}

void UINewFile::draw() {
    ImGuiStyle& style = ImGui::GetStyle();

    ImGui::Columns(2);

    ImGui::SetColumnWidth(0, 80.0f);
    ImGui_DrawProperties("Name:", &mName);

    ImGui::NextColumn();
    std::string directory = mPath.string();
    ImGui_DrawProperties("Path:", &directory);

    mPath = directory;

    ImGui::Columns(1);

    float width = ImGui::GetWindowWidth();
    float height = ImGui::GetWindowHeight();

    ImGui::SetCursorPosX(width - 190);
    ImGui::SetCursorPosY(height - 45);

    if (ImGui::Button("Ok", ImVec2(80, 25))) {
        create();
        close();
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel", ImVec2(80, 25))) {
        close();
    }
}

void UIProjectProperties::open(std::shared_ptr<Project> proj) {
    mProject = proj;
    mDirectory = mProject->getDirectoryPath().string();
    mObjDirectory = mProject->getObjPath();
    mBinDirectory = mProject->getBinPath();

    // Copy the current properties
    mProperties = mProject->mDesc;

    mShow = true;
    ImGui::OpenPopup("Project Properties");
}

void UIProjectProperties::applyChanges() {
    if (!mProperties.mName.empty()) {
        PaperCode::get().updateProjectProperties(mProperties);
    } else {

    }
}

void UIProjectProperties::close() {
    mProject = nullptr;
    mShow = false;
    ImGui::CloseCurrentPopup();
}

void UIProjectProperties::draw() {
    if (mProject) {
        ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_AutoSelectNewTabs;
        if (ImGui::BeginTabBar("PropertiesTabs", tab_bar_flags)) {

            if (ImGui::BeginTabItem("General")) {


                ImGui_DrawProperties("Name:", &mProperties.mName);
                ImGui_DrawProperties("Path:", &mDirectory, 0.0f, true);
                ImGui_DrawProperties("Object Path:", &mObjDirectory);
                ImGui_DrawProperties("Output Path:", &mBinDirectory);

                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Compiler Options")) {

                ImGui_Select<BuildLanguageStandard>(mProperties.mBuildOption.mLanguageStandard, BuildLanguageStandardString, "Language Standard");
                ImGui_DrawProperties("Additional Flags:", &mProperties.mBuildOption.mAdditionalCompileFlags);

                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Linker Options")) {

                ImGui_Select<BuildSubSystem>(mProperties.mBuildOption.mSubSystem, BuildSubSystemString, "Sub System");
                ImGui_Select<BuildType>(mProperties.mBuildOption.mType, BuildTypeString, "Build As");

                ImGui_DrawProperties("Additional Flags:", &mProperties.mBuildOption.mAdditionalLinkFlags);

                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        float width = ImGui::GetWindowWidth();
        float height = ImGui::GetWindowHeight();

        ImGui::SetCursorPosX(width - 190);
        ImGui::SetCursorPosY(height - 45);

        if (ImGui::Button("Ok", ImVec2(80, 25))) {
            applyChanges();
            close();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(80, 25))) {
            close();
        }
    }
}

