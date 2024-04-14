#pragma once

ImVec2 ImGui_DrawProperties(const std::string& caption, std::string* text, float space = 0.0f, bool readOnly = false, bool twoColumn = true);
void ImGui_QuickTooltip(const std::string& tip, ImFont* font);
bool ImGui_LinkButton(const std::string& text, const std::string& tooltip = "");
void ImGui_AlignWidth(float width, float alignment = 0.5f);
void ImGui_Align(float width, float height, float alignment = 0.5f);
void ImGui_TextCentered(const std::string& text);

void ImGuiSetTheme();

template<typename T>
void ImGui_Select(T& options, const char* stringlist[], const std::string& title) {
    T standard = options;
    const char* currentTypeString = stringlist[int(standard)];

    ImGui::Columns(2);

    ImGui::SetColumnWidth(0, 170.0f);

    //ImGui::ItemLabel("Language Standard", ItemLabelFlag::Left);
    ImGui::AlignTextToFramePadding();
    ImGui::Text("%s", title.c_str());

    ImGui::NextColumn();

    std::string combo_id = std::format("##{}", title); 

    if (ImGui::BeginCombo(combo_id.c_str(), currentTypeString)) {

        for(int i = 0; i< int(T::MAX);i++) {

            bool isSelected = currentTypeString == stringlist[i];

            if (ImGui::Selectable(stringlist[i], isSelected)) {
                currentTypeString = stringlist[i];
                options = (T)i;
            }

            if (isSelected) {
                ImGui::SetItemDefaultFocus();
            }
        }

        ImGui::EndCombo();
    }

    ImGui::Columns(1);
}
