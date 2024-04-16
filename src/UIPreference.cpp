#include "Stdafx.h"
#include "PaperCode.h"
#include "ImGuiHelper.h"

void UIPreference::open() {
    
    ImGui::OpenPopup("Preference");
}

void UIPreference::applyChanges() {
    
}

void UIPreference::close() {
    ImGui::CloseCurrentPopup();
}

void UIPreference::draw() {
    ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_AutoSelectNewTabs;
    if (ImGui::BeginTabBar("PreferenceTabs", tab_bar_flags)) {

        if (ImGui::BeginTabItem("General")) {
            

            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Toolchain")) {

            

            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Editor")) {

            

            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Theme")) {

            

            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("File")) {

            

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
