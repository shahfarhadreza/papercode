#include "Stdafx.h"

#include <imgui_internal.h>

#include "ImGuiHelper.h"

void ImGui_QuickTooltip(const std::string& tip, ImFont* font) {
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_None)) {
        ImGui::PushFont(font);
        ImGui::SetTooltip("%s", tip.c_str());
        ImGui::PopFont();
    }
}

void ImGui_TextCentered(const std::string& text) {
    float win_width = ImGui::GetWindowSize().x;
    float text_width = ImGui::CalcTextSize(text.c_str()).x;

    // calculate the indentation that centers the text on one line, relative
    // to window left, regardless of the `ImGuiStyleVar_WindowPadding` value
    float text_indentation = (win_width - text_width) * 0.5f;

    // if text is too long to be drawn on one line, `text_indentation` can
    // become too small or even negative, so we check a minimum indentation
    float min_indentation = 20.0f;
    if (text_indentation <= min_indentation) {
        text_indentation = min_indentation;
    }

    ImGui::SameLine(text_indentation);
    ImGui::PushTextWrapPos(win_width - text_indentation);
    ImGui::TextWrapped("%s", text.c_str());
    ImGui::PopTextWrapPos();
}

ImVec2 ImGui_DrawProperties(const std::string& caption, std::string* text, float space, bool readOnly, bool twoColumn) {

    if (twoColumn) {
        ImGui::Columns(2);
        ImGui::SetColumnWidth(0, 170.0f);
    }

    ImGui::AlignTextToFramePadding();
    ImGui::Text("%s", caption.c_str());

    if (twoColumn) {
        ImGui::NextColumn();
    } else {
        ImGui::SameLine();
    }

    ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - space);
    if (readOnly) {
        ImGui::AlignTextToFramePadding();
        ImGui::Text("%s", text->c_str());
    } else {
        char buffer[256];
        memset(buffer, 0, sizeof(buffer));
#if defined(WIN32)
        strncpy_s(buffer, sizeof(buffer), (*text).c_str(), sizeof(buffer));
#else
        strncpy(buffer, (*text).c_str(), sizeof(buffer));
#endif
        if (ImGui::InputText(("##" + caption).c_str(), buffer, sizeof(buffer))) {
            *text = std::string(buffer);
        }
    }
    ImGui::PopItemWidth();

    if (twoColumn) {
        ImGui::Columns(1);
    }

    return ImGui::CalcTextSize(caption.c_str());
}


void ImGui_Align(float width, float height, float alignment) {
    ImVec2 avail = ImGui::GetContentRegionAvail();

    float offX = (avail.x - width) * alignment;
    float offY = (avail.y - height) * alignment;
    if (offX > 0.0f) {
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offX);
    }
    if (offY > 0.0f) {
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + offY);
    }
}

void ImGui_AlignWidth(float width, float alignment) {
    ImVec2 avail = ImGui::GetContentRegionAvail();

    float offX = (avail.x - width) * alignment;
    if (offX > 0.0f) {
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offX);
    }
}

bool ImGui_LinkButton(const std::string& text, const std::string& tooltip) {
    ImGui::TextColored(ImVec4(0.15, 0.55, 1, 1), "%s", text.c_str());

    if (ImGui::IsItemHovered(ImGuiHoveredFlags_None)) {
        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
        if (!tooltip.empty()) {
            ImGui::SetTooltip("%s", tooltip.c_str());
        }
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            return true;
        }
    }
    return false;
}

void ImGuiSetTheme()  {
    ImGuiStyle& style = ImGui::GetStyle();

    style.WindowMenuButtonPosition = 0;
    style.WindowPadding.x = 5;
    style.WindowRounding = 3.0f;
	style.WindowBorderSize = 1;

    style.TabRounding = 3.0f;

	style.ScrollbarSize = 10;
	style.ScrollbarRounding = 15;
	style.PopupBorderSize = 0;

    style.FrameBorderSize = 1;
	style.FrameRounding = 3.0f;

    

	style.Colors[ImGuiCol_Separator] = ImVec4(0.01f, 0.01f, 0.01f, 1.00f);

	style.Colors[ImGuiCol_Text] = ImVec4(0.8f, 0.8f, 0.8f, 1.00f);
	style.Colors[ImGuiCol_TitleBg] = ImVec4(0.17f, 0.17f, 0.17f, 1.00f);
	style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
	style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.23f, 0.23f, 0.23f, 1.00f);
	style.Colors[ImGuiCol_Header] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
	style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);

	style.Colors[ImGuiCol_Button] = ImVec4(0.19f, 0.19f, 0.19f, 1.00f);
	style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.25f, 0.25f, 0.32f, 1.00f);
	style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.17f, 0.17f, 0.17f, 1.00f);

	style.Colors[ImGuiCol_WindowBg] = ImVec4(0.135f, 0.14f, 0.135f, 1.00f);
	style.Colors[ImGuiCol_FrameBg] = ImVec4(0.09f, 0.09f, 0.09f, 1.f);
	style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.15f, 0.15f, 0.15f, 1.f);
	style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.17f, 0.17f, 0.17f, 1.00f);
	style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.17f, 0.17f, 0.17f, 1.00f);
	style.Colors[ImGuiCol_Tab] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
	style.Colors[ImGuiCol_Border] = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);

	style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0, 0, 0, 0.4f);
	style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.f, 0.f, 0.f, 0.f);
	style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.f, 0.f, 0.f, 0.f);
	style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.f, 0.f, 0.f, 0.f);

	style.Colors[ImGuiCol_ChildBg] = ImVec4(0.135f, 0.135f, 0.135f, 1.00f);

	style.Colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 1.00f);
}

