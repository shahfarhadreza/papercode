#include "Stdafx.h"
#include "PaperCode.h"

#include "ImGuiHelper.h"

void UIEditorManager::saveActive() {
	if (mActiveEditor) {
		mActiveEditor->saveToFile();
	}
}

void UIEditorManager::saveAll() {
	for(const UIEditorPtr& e : mEditors) {
        e->saveToFile();
    }
}

UIEditorPtr UIEditorManager::getEditor(const std::filesystem::path& filepath) {
	UIEditorPtr editor = nullptr;
    for (UIEditorPtr e : mEditors) {
        if (e->isFileOpen(filepath)) {
            editor = e;
            break;
        }
    }
    return editor;
}

UIEditorPtr UIEditorManager::openEditor(const std::filesystem::path& filepath) {
    UIEditorPtr editor = getEditor(filepath);
    if (!editor) {
        editor = std::make_shared<UIEditor>();
        editor->init();
        editor->openFile(filepath);
        mEditors.push_back(editor);
    } else {
        // We should switch to that tab
        editor->mFlagSelected = true;
    }
    return editor;
}

void UIEditorManager::closeEditor(const std::string& filepath) {
    UIEditorList::iterator it;

    for (it = mEditors.begin(); it != mEditors.end();++it) {
        UIEditorPtr e = *it;
        if (e->isFileOpen(filepath)) {
            break;
        }
    }

    if (it != mEditors.end()) {
        UIEditorPtr editor = (*it);
        if (editor->isModified()) {
            UISystem::get().messageBox(std::format("{} has been modified, save changes?", editor->getFileName()), 
                UIMessageBoxType::YesNoCancel, [this, editor, it](auto action) { 
                    if (action == UIMessageBoxAction::Yes) {
                        editor->saveToFile();
                        editor->destroy();
                        mEditors.erase(it);
                    } else if (action == UIMessageBoxAction::No) {
                        editor->destroy();
                        mEditors.erase(it);
                    }
                });
        } else {
            editor->destroy();
            mEditors.erase(it);
        }
        if (mActiveEditor == editor) {
            mActiveEditor = nullptr;
        }
    }
}

void UIEditorManager::closeEditor(UIEditorPtr editor) {
    auto it = std::find(mEditors.begin(), mEditors.end(), editor);
    if (it != mEditors.end()) {
        UIEditorPtr editor = (*it);
        if (editor->isModified()) {
            UISystem::get().messageBox(std::format("{} has been modified, save changes?", editor->getFileName()), 
                UIMessageBoxType::YesNoCancel, [this, editor, it](auto action) {
                    if (action == UIMessageBoxAction::Yes) {
                        editor->saveToFile();
                        editor->destroy();
                        mEditors.erase(it);
                    } else if (action == UIMessageBoxAction::No) {
                        editor->destroy();
                        mEditors.erase(it);
                    }
                });
        } else {
            editor->destroy();
            mEditors.erase(it);
        }
    }
    if (mActiveEditor == editor) {
        mActiveEditor = nullptr;
    }
}

void UIEditorManager::closeAllEditors() {
    UIEditorList::iterator it = mEditors.begin();
    bool isModified = false;
    while (it != mEditors.end()) {
        if ((*it)->isModified()) {
            isModified = true;
            ++it;
        } else {
            (*it)->destroy();
            it = mEditors.erase(it);
        }
    }
    if (isModified) {
        UISystem::get().messageBox("One or more file(s) have been modified, close them individualy", UIMessageBoxType::Information);
    } else {
        mEditors.clear();
    }
    mActiveEditor = nullptr;
}

void UIEditorManager::draw() {
	ImGuiWindowClass window_class;
    window_class.DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_AutoHideTabBar;
    ImGui::SetNextWindowClass(&window_class);

    ImGui::Begin("TextEditors", &mShowEditors);

    UIEditorPtr closeTab = nullptr;
    enum class TabAction {
        None,
        Close,
        CloseAll
    };
    TabAction action = TabAction::None;

    if (!mEditors.empty()) {

        ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_AutoSelectNewTabs;
        if (ImGui::BeginTabBar("TextEditorsTabs", tab_bar_flags)) {
            char id[256];
            int n = 0;
            for (UIEditorPtr e : mEditors) {
                std::string title = "";

                ImGuiTabItemFlags tab_flags = 0;

                if (e->mImEditor->IsTextChanged()) {
                    if (e->mFirstLoaded == true) {
                        //e.mProjectFile->setModified(false);
                        e->mImEditor->Render("TextEditor");
                        e->mFirstLoaded = false;
                    } else {
                        e->setModified(true);
                    }
                }
                title = e->getFileName();

                if (e->isModified()) {
                    tab_flags |= ImGuiTabItemFlags_UnsavedDocument;
                }

                if (e->mFlagSelected) {
                    tab_flags |= ImGuiTabItemFlags_SetSelected;
                    e->mFlagSelected = false;
                }

                if (ImGui::BeginTabItem((ICON_FA_FILE_CODE  " " + title).c_str(), nullptr, tab_flags)) {

                    if (ImGui::BeginPopupContextItem("Editor Tab Context Menu")) {
                        if (ImGui::MenuItem(ICON_FA_BOOK "Close")) {
                            closeTab = e;
                            action = TabAction::Close;
                        }
                        if (ImGui::MenuItem(ICON_FA_BOOK "Close All")) {
                            action = TabAction::CloseAll;
                        }
                        ImGui::EndPopup();
                    }

                    if (ImGui::IsItemHovered(ImGuiHoveredFlags_None)) {
                        ImGui::SetTooltip("%s", e->getFilePath().string().c_str());
                    }

                    if (mActiveEditor != e) {
                    	UISystem::get().updateTitle(title + " - PaperCode");
                    }
                    mActiveEditor = e;

                    //ImGui::Begin(title.c_str(), nullptr);//, nullptr, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_MenuBar);
                    e->mImEditor->Render("TextEditor");
                    //ImGui::End();
                    ImGui::EndTabItem();
                }
                n++;
            }

            ImGui::EndTabBar();
        }
    }

    ImGui::End();

    if (action == TabAction::Close) {
        closeEditor(closeTab);
        mActiveEditor = nullptr;
    } else if (action == TabAction::CloseAll) {
        closeAllEditors();
        mActiveEditor = nullptr;
    }
}

