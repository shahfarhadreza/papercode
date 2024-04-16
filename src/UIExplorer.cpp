#include "Stdafx.h"
#include "PaperCode.h"
#include "ImGuiHelper.h"

enum class ProjectAction {
    None,
    NewFile,
    AddExisting,
    Rename,
    Properties,
    Close,
};

FileContextMenuAction UIExplorer::drawTreeItem(const std::string& name, ProjectFilePtr file, ProjectPtr project) {
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_Leaf;

    std::filesystem::path filePath = file->getAbsolutePath(project);

    if (UISystem::get().getEditorManager().mActiveEditor && UISystem::get().getEditorManager().mActiveEditor->isFileOpen(filePath.string())) {
        flags |= ImGuiTreeNodeFlags_Selected;
    }

    bool expanded = ImGui::TreeNodeEx((void*)file.get(), flags, "%s", name.c_str());

    if (expanded) {

        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && ImGui::IsItemHovered(ImGuiHoveredFlags_None)) {
            UISystem::get().getEditorManager().openEditor(filePath);
        }
    }

    FileContextMenuAction action = FileContextMenuAction::None;

    if (ImGui::BeginPopupContextItem("Project File Context Menu")) {

        // TODO: Is this the correct way?
        UISystem::get().getEditorManager().openEditor(filePath);

        if (ImGui::MenuItem(ICON_FA_SAVE " Save")) {
            action = FileContextMenuAction::Save;
        }
        if (ImGui::MenuItem(ICON_FA_BOOK " Close")) {
            action = FileContextMenuAction::Close;
        }
        ImGui::Separator();
        if (ImGui::MenuItem(ICON_FA_EDIT " Rename...")) {
            action = FileContextMenuAction::Rename;
        }
        if (ImGui::MenuItem(ICON_FA_MINUS " Remove From Project")) {
            action = FileContextMenuAction::Remove;
        }
        if (ImGui::MenuItem(ICON_FA_TRASH " Delete File")) {
            action = FileContextMenuAction::Delete;
        }
        ImGui::EndPopup();
    }

    if (expanded) {
        ImGui::TreePop();
    }

    return action;
}

void UIExplorer::draw() {

    ImGui::Begin("Explorer");

    PaperCode& app = PaperCode::get();

    ProjectAction action = ProjectAction::None;

    auto mProject = app.getActiveProject();

    if (mProject) {
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_DefaultOpen;

        bool expanded = ImGui::TreeNodeEx((void*)mProject.get(), flags, "%s", mProjectNodeText.c_str());
            //ImGui::SeparatorText("General");

        if (ImGui::BeginPopupContextItem("Project Context Menu")) {
            if (ImGui::MenuItem(ICON_FA_FILE " New File")) {
                action = ProjectAction::NewFile;
            }
            if (ImGui::MenuItem(ICON_FA_FILE_ALT " Add Existing File")) {
                action = ProjectAction::AddExisting;
            }
            if (ImGui::MenuItem(ICON_FA_EDIT " Rename...")) {
                action = ProjectAction::Rename;
            }
            if (ImGui::MenuItem(ICON_FA_BOOK " Close")) {
                action = ProjectAction::Close;
            }
            ImGui::EndPopup();
        }

        if (action == ProjectAction::NewFile) {
            PaperCode::get().executeCommand(Commands::NewFile);
        }

        if (action == ProjectAction::AddExisting) {
            std::string filePath = UISystem::get().openFileDialog("C++ Source File (*.cpp;*.cxx)\0*.cpp;*.cxx\0C++ Header File (*.h)\0*.h\0"
                                     "All Files (*.*)\0*.*\0");
            if (!filePath.empty()) {
                PaperCode::get().executeCommand(Commands::AddExistingFile, {.FilePath = filePath});
            }
        }

        if (ImGui::BeginPopupModal("New File")) {
            UISystem::get().mNewFile.draw();
            ImGui::EndPopup();
        }

        if (ImGui::BeginPopupModal("Rename File")) {
            UISystem::get().mRenameFile.draw();
            ImGui::EndPopup();
        }
        
        ProjectFilePtr fileToRemove = nullptr;

        if (expanded) {
            mProjectNodeText = ICON_FA_FOLDER_OPEN " " + mProject->getName();

            for (ProjectFilePtr file : mProject->mFileList) {

                FileContextMenuAction action = drawTreeItem(ICON_FA_FILE_CODE  " " + file->getFileName(), file, mProject);

                switch(action) {
                case FileContextMenuAction::Close:
                    // TODO: UI's Job Only?
                    UISystem::get().getEditorManager().closeEditor(file->getPath());
                    break;
                case FileContextMenuAction::Rename:
                    UISystem::get().mRenameFile.open(file);
                    break;
                case FileContextMenuAction::Save:
                    // Lets just assume this file is opened in the active editor (since right click also opens the file)
                    // TODO: Do it in a better way (Find the corrosponding editor of this file and ask it to save the content)
                    if (UISystem::get().getEditorManager().mActiveEditor && UISystem::get().getEditorManager().mActiveEditor->isFileOpen(file->getPath())) {
                        UISystem::get().getEditorManager().mActiveEditor->saveToFile();
                    }
                    break;
                case FileContextMenuAction::Remove:
                    // TODO: What if user wants to select and remove multiple files?
                    assert(fileToRemove == nullptr); // ???
                    fileToRemove = file;
                    break;
                case FileContextMenuAction::Delete:
                    break;
                case FileContextMenuAction::None:
                    // Do nothing
                    break;
                }
            }
        }

        if (expanded) {
            ImGui::TreePop();
            ImGui::Spacing();
        } else {
            mProjectNodeText = ICON_FA_FOLDER " " + mProject->getName();
        }

        // TODO: What if multiple files?
        if (fileToRemove) {
            UISystem::get().messageBox(std::format("Are you sure you want remove '{}'?", fileToRemove->getFileName()), 
                UIMessageBoxType::YesNo, [fileToRemove](auto action) {
                    if (action == UIMessageBoxAction::Yes) {
                        PaperCode::get().executeCommand(Commands::RemoveProjectFile, {.File = fileToRemove});
                    }
                });
        }

        if (action == ProjectAction::Close) {
            app.closeProject();
        }
    }

    if (ImGui::BeginPopupModal((UISystem::get().mMsgBox.mTitle + "###Message Box").c_str(), 0, 0/*ImGuiWindowFlags_NoResize*/)) {
        UISystem::get().mMsgBox.draw();
        ImGui::EndPopup();
    }

    ImGui::End();
}

