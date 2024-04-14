#include "Stdafx.h"
#include "PaperCode.h"
#include "ImGuiHelper.h"

void UINewProject::open() {
    mDesc.mName = "HelloWorld";

    char* pUserDir = std::getenv("USERPROFILE");

    if (pUserDir != nullptr) {

        std::filesystem::path defaultProjectDir = std::string(pUserDir);

        defaultProjectDir.append("PaperCode");
        defaultProjectDir.append(mDesc.mName);

        mDirectory = defaultProjectDir;

        // TODO: Add support for path variables ${PROJECTDIR} etc. (Maybe use Regex to implement it)
        mDesc.mObjPath = ".\\obj";
        mDesc.mBinPath = ".\\bin";

        ImGui::OpenPopup("New Project");
    } else {
        std::cout << "CRITICAL ERROR: Could not find default project location" << std::endl;
    }
}

void UINewProject::close() {
    ImGui::CloseCurrentPopup();
}

void UINewProject::create() {

	ProjectTemplate helloWorldTemp;

    if (mAddHelloWorldTemplate) {
	   std::string code = 
R"(#include <iostream>

int main(int argc, char** argv) {
    std::cout << "Hello World! Welcome To Paper Code!!" << std::endl;
    return 0;
}
)";
        helloWorldTemp.addTemplateFile("main.cpp", code);
    }

    mDesc.mFilePath = std::filesystem::path(mDirectory);
    mDesc.mFilePath.append(mDesc.mName + ".paper");

	helloWorldTemp.mDesc = mDesc;

    PaperCode::get().createProject(helloWorldTemp);
}

void UINewProject::draw() {
    ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_AutoSelectNewTabs;
    ImGuiStyle& style = ImGui::GetStyle();

    ImGui_DrawProperties("Name:", &mDesc.mName);

    std::string projDirectory = mDirectory.string();

    ImGui_DrawProperties("Path:", &projDirectory, 30 + style.ItemSpacing.x);
    ImGui::SameLine();
    if (ImGui::Button("...", ImVec2(30, 0))) {

    }

    mDirectory = projDirectory;

    ImGui_DrawProperties("Object Path:", &mDesc.mObjPath, 30 + style.ItemSpacing.x, true);

    ImGui_DrawProperties("Output Path:", &mDesc.mBinPath, 30 + style.ItemSpacing.x, true);

    ImGui::Separator();

    ImGui_Select<BuildLanguageStandard>(mDesc.mBuildOption.mLanguageStandard, BuildLanguageStandardString, "Language Standard");
    ImGui_Select<BuildSubSystem>(mDesc.mBuildOption.mSubSystem, BuildSubSystemString, "Sub System");
    ImGui_Select<BuildType>(mDesc.mBuildOption.mType, BuildTypeString, "Build As");

    if (mDesc.mBuildOption.mType == BuildType::Executable) {
        ImGui::Checkbox("Hello World Template", &mAddHelloWorldTemplate);
    }

    float width = ImGui::GetWindowWidth();
    float height = ImGui::GetWindowHeight();

    ImGui::SetCursorPosX(width - 190);
    ImGui::SetCursorPosY(height - 45);

    if (ImGui::Button("Create", ImVec2(80, 25))) {
        create();
        close();
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel", ImVec2(80, 25))) {
        close();
    }
}

