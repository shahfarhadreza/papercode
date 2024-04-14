#include "Stdafx.h"
#include "PaperCode.h"
#include "ImGuiHelper.h"

void UIMessageBox::open(const std::string& title, const std::string& msg, UIMessageBoxType type, UIMsgBoxFn cb) {
    mTitle = title;
    mMsg = msg;
    mType = type;
    mFunc = cb;

    ImGui::OpenPopup((title + "###Message Box").c_str());
}

void UIMessageBox::close() {
    ImGui::CloseCurrentPopup();
}

void UIMessageBox::draw() {
    const ImGuiStyle& style = ImGui::GetStyle();

    ImGui::Columns(2, 0, false);

    ImGui::SetColumnWidth(0, 50.0f);

    ImGui::PushFont(UISystem::get().mDefaultLargeFontGUI);

    //ImGui::AlignTextToFramePadding();

    float winHeight = ImGui::GetContentRegionAvail().y;

    const char* textIcon = ICON_FA_EXCLAMATION;
    ImVec4 iconColor = ImVec4(0.8, 0.15, 0.15, 1);

    if (mType == UIMessageBoxType::YesNo || mType == UIMessageBoxType::YesNoCancel) {
        textIcon = ICON_FA_QUESTION;
        iconColor = ImVec4(0.8, 0.6, 0.0, 1);
    }

    float iconHeight = ImGui::CalcTextSize(textIcon).y;

    float ioffX = (winHeight - iconHeight) * 0.5;
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + ioffX - (iconHeight/2));

    ImGui::TextColored(iconColor, "%s", textIcon);
    ImGui::PopFont();

    ImGui::NextColumn();

    //ImGui::AlignTextToFramePadding();

    ImGui_TextCentered(mMsg);

    ImGui::Columns(1);

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + style.WindowPadding.y);

    if (mType == UIMessageBoxType::YesNo) {
        ImVec2 avail = ImGui::GetContentRegionAvail();

        float offX = (avail.x - 160) * 0.5;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offX);

        if (ImGui::Button("Yes", ImVec2(80, 25))) { 
            if (mFunc) {
                mFunc(UIMessageBoxAction::Yes);
            }
            close();
        }
        ImGui::SameLine();
        if (ImGui::Button("No", ImVec2(80, 25))) {
            if (mFunc) {
                mFunc(UIMessageBoxAction::No);
            }
            close();
        }
    } else if (mType == UIMessageBoxType::YesNoCancel) {
        ImVec2 avail = ImGui::GetContentRegionAvail();

        float offX = (avail.x - 240) * 0.5;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offX);

        if (ImGui::Button("Yes", ImVec2(80, 25))) {
            if (mFunc) {
                mFunc(UIMessageBoxAction::Yes);
            }
            close();
        }
        ImGui::SameLine();
        if (ImGui::Button("No", ImVec2(80, 25))) {
            if (mFunc) {
                mFunc(UIMessageBoxAction::No);
            }
            close();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(80, 25))) {
            if (mFunc) {
                mFunc(UIMessageBoxAction::Cancel);
            }
            close();
        }
    } else {
        ImVec2 avail = ImGui::GetContentRegionAvail();

        float offX = (avail.x - 80) * 0.5;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offX);

        if (ImGui::Button("Ok", ImVec2(80, 25))) {
            if (mFunc) {
                mFunc(UIMessageBoxAction::Yes);
            }
            close();
        }
    }
}

