#include "Stdafx.h"
#include "PaperCode.h"

#define STBI_WINDOWS_UTF8
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include <imgui_internal.h>

#include "ImGuiHelper.h"

std::string OpenFileDialog(GLFWwindow* window, const char* filters);
bool glfwSetWindowCenter( GLFWwindow * window );

void GLFW_error(int error, const char* description) {
    std::cout << "GLFW ERROR(" << error << "):" << description << std::endl;
}

bool UISystem::init(uint32_t height, uint32_t width, const char* title) {
    glfwInit();
    glfwSetErrorCallback(GLFW_error);
	//glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
	//glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);

    mTitle = title;

	mWindow = glfwCreateWindow(width, height, title, nullptr, nullptr);
	if (mWindow == nullptr) {
		std::cout << "ERROR::WINDOW::CREATION" << std::endl;
		glfwTerminate();
		return false;
	}
	glfwMakeContextCurrent(mWindow);
	glfwSetWindowCenter(mWindow);

	// GLAD
    std::cout << "LOG: Glad GL Loader...." << std::endl;
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cout << "ERROR: Failed to initialize GLAD" << std::endl;
		glfwTerminate();
		return false;
	}

    std::cout << "OpenGL: " << glGetString(GL_VERSION) << std::endl;

	int w, h, c;
	unsigned char* data = stbi_load("./logo/logo.png", &w, &h, &c, STBI_rgb_alpha);

	if (data) {
        GLFWimage images[1];
        images[0].width = w;
        images[0].height = h;
        images[0].pixels = data;
        glfwSetWindowIcon(mWindow, 1, images);
        // we don't need to keep the data in ram,
        stbi_image_free(data);
    } else {
        std::cout << "ERROR: Failed to load icon './logo/logo.png'" << std::endl;
    }
	//const char* glsl_version = "#version 210";
	// Setup Dear ImGui context
    std::cout << "LOG: Initializing ImGUI...." << std::endl;
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    ImGuiSetTheme();

    // Font Setup
    initFonts();

    // Setup Platform/Renderer backends
    std::cout << "LOG: Setup ImGUI OpenGL Context...." << std::endl;
    ImGui_ImplGlfw_InitForOpenGL(mWindow, true);
    ImGui_ImplOpenGL2_Init();

	return true;
}

void UISystem::initFonts() {
    ImGuiIO& io = ImGui::GetIO();

    float baseFontSize;
    float iconFontSize;

    static const ImWchar icons_ranges[] = { ICON_MIN_FA, ICON_MAX_16_FA, 0 };
    ImFontConfig icons_config;
    icons_config.MergeMode = true;
    icons_config.PixelSnapH = true;

    // Default Size
    baseFontSize = 15.0f; // 13.0f is the size of the default font. Change to the font size you use.
    iconFontSize = baseFontSize * 2.8f / 3.0f; // FontAwesome fonts need to have their sizes reduced by 2.0f/3.0f in order to align correctly

    mDefaultFontGUI = io.Fonts->AddFontFromFileTTF("./fonts/Consolas.ttf", baseFontSize);
    // merge in icons from Font Awesome
    icons_config.GlyphMinAdvanceX = iconFontSize;
    io.Fonts->AddFontFromFileTTF( "./fonts/fa-solid-900.ttf", iconFontSize, &icons_config, icons_ranges );

    // Mid Size
    baseFontSize = 22.0f; // 13.0f is the size of the default font. Change to the font size you use.
    iconFontSize = baseFontSize * 2.8f / 3.0f; // FontAwesome fonts need to have their sizes reduced by 2.0f/3.0f in order to align correctly

    mDefaultMidFontGUI = io.Fonts->AddFontFromFileTTF("./fonts/Consolas.ttf", baseFontSize);
    // merge in icons from Font Awesome
    icons_config.GlyphMinAdvanceX = iconFontSize;
    io.Fonts->AddFontFromFileTTF( "./fonts/fa-solid-900.ttf", iconFontSize, &icons_config, icons_ranges );

    // Large Size
    baseFontSize = 42.0f; // 13.0f is the size of the default font. Change to the font size you use.
    iconFontSize = baseFontSize * 2.8f / 3.0f; // FontAwesome fonts need to have their sizes reduced by 2.0f/3.0f in order to align correctly

    mDefaultLargeFontGUI = io.Fonts->AddFontFromFileTTF("./fonts/Consolas.ttf", baseFontSize);
    // merge in icons from Font Awesome
    icons_config.GlyphMinAdvanceX = iconFontSize;
    io.Fonts->AddFontFromFileTTF( "./fonts/fa-solid-900.ttf", iconFontSize, &icons_config, icons_ranges );
}

void UISystem::newFileDialog(ProjectPtr project) {
    if (project) {
        mNewFile.open(project);
    }
}

void UISystem::messageBox(const std::string& msg, UIMessageBoxType type, UIMsgBoxFn cb) {
    mMsgBox.open(mTitle, msg, type, cb);
}

void UISystem::notify(UINotification notif, UINotificationData data) {
    switch(notif) {
    case UINotification::ProjectFileRemoved:
        // The file object is already gone, left us the path to match with
        if (!data.FilePath.empty()) {
            // We need to close any editor ascociated with the removed file
            getEditorManager().closeEditor(data.FilePath);
        }
        break;
    case UINotification::ProjectFileRenamed:
        // The file object is already gone, left us the path to match with
        if (!data.FilePath.empty()) {
            // We need to update the editor's title ascociated with the renamed file
            UIEditorPtr editor = getEditorManager().getEditor(data.FilePath);
            if (editor) {
                editor->updateFilePath(data.FileNewPath);
            } else {
                // File wasn't opened in any editor, so nothing to do
            }
        }
        break;
    default:
        break;
    }
}

bool UISystem::runEventLoop() {
    //printf("runEventLoop\n");
    while (!glfwWindowShouldClose(mWindow)) {

        int width, height;
        glfwGetWindowSize(mWindow, &width, &height);
        glViewport(0, 0, width, height);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        drawEvent();
	    glfwPollEvents();

        glfwSwapBuffers(mWindow);
	}
    return true;
}

void UISystem::clearBuildLogs() {
    mBuildLogs.clear();
}

void UISystem::appendBuildLog(const std::string& log, LogType type) {
    mBuildLogs.log(log, type);
}

void UISystem::drawToolbar() {
    ImVec2 toolBtnSize = ImVec2(40, 36);

    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));

    if (ImGui::Button(ICON_FA_FILE, toolBtnSize)) {
        PaperCode::get().executeCommand(Commands::NewFile);
    }
    ImGui_QuickTooltip("New Project/File", mDefaultFontGUI);
    //ImGui::SameLine();
    if (ImGui::Button(ICON_FA_FOLDER_OPEN, toolBtnSize)) {

    }
    ImGui_QuickTooltip("Open Project/File", mDefaultFontGUI);
    //ImGui::SameLine();
    if (ImGui::Button(ICON_FA_SAVE, toolBtnSize)) {
        PaperCode::get().executeCommand(Commands::SaveCurrent);
    }
    ImGui_QuickTooltip("Save Current File...", mDefaultFontGUI);

    //ImGui::SameLine();
    if (ImGui::Button(ICON_FA_SD_CARD, toolBtnSize)) {
        PaperCode::get().executeCommand(Commands::SaveAll);
    }
    ImGui_QuickTooltip("Save Everything...", mDefaultFontGUI);

    // We won't need the following button unless we have a project
    if (PaperCode::get().getActiveProject()) {
        bool disbaled = false;

        if (PaperCode::get().isBuilding()) {
            disbaled = true;
        }

        if (disbaled) {
            ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
        }
        //ImGui::SameLine();
        if (ImGui::Button(ICON_FA_HAMMER, toolBtnSize)) {
            PaperCode::get().executeCommand(Commands::Build);
        }
        ImGui_QuickTooltip("Build Active Project", mDefaultFontGUI);
        //ImGui::SameLine();
        if (PaperCode::get().isProjectRunning()) {
            if (ImGui::Button(ICON_FA_STOP, toolBtnSize)) {
                PaperCode::get().executeCommand(Commands::Stop);
            }

        } else {
            if (ImGui::Button(ICON_FA_PLAY, toolBtnSize)) {
                PaperCode::get().executeCommand(Commands::Run);
            }
        }
        ImGui_QuickTooltip("Run Project", mDefaultFontGUI);

        if (disbaled) {
            ImGui::PopItemFlag();
            ImGui::PopStyleVar();
        }

        //ImGui::SameLine();
        if (ImGui::Button(ICON_FA_USER_COG, toolBtnSize)) {
            PaperCode::get().executeCommand(Commands::ProjectProperties);
        }
        ImGui_QuickTooltip("Project Properties", mDefaultFontGUI);
    }
    //ImGui::SameLine();
    if (ImGui::Button(ICON_FA_COG, toolBtnSize)) {
        mPreference.open();
        //PaperCode::get().executeCommand(Commands::Settings);
    }
    ImGui_QuickTooltip("Preferences", mDefaultFontGUI);

    ImGui::PopStyleColor();
    ImGui::PopStyleColor();
}

void UISystem::drawUI() {

    ImGui::Begin("toolbar", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar);

    ImGui::PushFont(mDefaultMidFontGUI);

    drawToolbar();

    ImGui::PopFont();

    if (ImGui::BeginPopupModal("Project Properties")) {
        mProjectProperties.draw();

        ImGui::EndPopup();
    }

    if (mNewFile.mTriggerPopup) {
        mNewFile.mTriggerPopup = false;
    }

    if (ImGui::BeginPopupModal("New File")) {
        mNewFile.draw();
        ImGui::EndPopup();
    }

    if (ImGui::BeginPopupModal("Preference")) {
        mPreference.draw();
        ImGui::EndPopup();
    }

    if (ImGui::BeginPopupModal((mMsgBox.mTitle + "###Message Box").c_str(), 0, 0/*ImGuiWindowFlags_NoResize*/)) {
        mMsgBox.draw();
        ImGui::EndPopup();
    }

    ImGui::End();

    ImGui::PushFont(mDefaultFontGUI);

    if (PaperCode::get().getActiveProject()) {
        mExplorer.draw();

        // Build Log
        ImGui::Begin(ICON_FA_HAMMER " Build Log");

        for(const LogEntry& log : mBuildLogs.mEntries) {

            const ImGuiStyle& style = ImGui::GetStyle();

            ImVec4 logColor;

            switch(log.Type) {
            case LogType::General:
                logColor = style.Colors[ImGuiCol_Text];
                break;
            case LogType::Info:
                logColor = ImVec4(0.15, 0.45, 1, 1);
                break;
            case LogType::Success:
                logColor = ImVec4(0.15, 1, 0.15, 1);
                break;
            case LogType::Error:
                logColor = ImVec4(1, 0.15, 0.15, 1);
                break;
            default:
                logColor = style.Colors[ImGuiCol_Text];
                break;
            }

            ImGui::PushStyleColor(ImGuiCol_Text, logColor);
            ImGui::TextWrapped("%s", log.Text.c_str());
            ImGui::PopStyleColor();
        }

        ImGui::End();
    }

    // Code Editors
    getEditorManager().draw();

    if (ImGui::BeginPopupModal((UISystem::get().mMsgBox.mTitle + "###Message Box").c_str(), 0, 0/*ImGuiWindowFlags_NoResize*/)) {
        UISystem::get().mMsgBox.draw();
        ImGui::EndPopup();
    }

    const ImGuiStyle& style = ImGui::GetStyle();

    // Status Bar
    if (mShowStatus) {
        ImGui::Begin("Status");

        if (PaperCode::get().isBuilding()) {
            ImGui::Text("%s", "Building...");
        } else {
            ImGui::Text("%s", "Ready");
        }

        UIEditorPtr editor = getEditorManager().getActiveEditor();

        if (editor) {
            ImGui::SameLine();

            std::string strLineColumn_ = std::format("Line: {}, Column: {}", editor->getLine(), editor->getColumn());
            const char* strLineColumn = strLineColumn_.c_str();
            float lineColumnW = ImGui::CalcTextSize(strLineColumn).x;

            std::string strTabSize_ = std::format("Tab Size: {}", editor->mImEditor->GetTabSize());
            const char* strTabSize = strTabSize_.c_str();
            float tabSizeW = ImGui::CalcTextSize(strTabSize).x;

            ImVec2 avail = ImGui::GetContentRegionAvail();
            ImGui::SetCursorPosX((ImGui::GetCursorPosX() + avail.x) - (tabSizeW + lineColumnW + (style.WindowPadding.x*5)));

            // Draw Them

            ImGui::Text("%s", strLineColumn);

            ImGui::SameLine();

            ImGui::SetCursorPosX((ImGui::GetCursorPosX() + (style.WindowPadding.x*2)));

            ImGui::Text("%s", strTabSize);
        }

        ImGui::End();
    }

    ImGui::PopFont();

}

void UISystem::updateTitle(const std::string& title) {
    glfwSetWindowTitle(mWindow, title.c_str());
}

void UISystem::drawUIStartPage() {
    ImGui::PushFont(mDefaultFontGUI);

    ImGui::Begin("Welcome", 0);

    ImVec2 sz = ImGui::GetContentRegionAvail();

    ImGui::Columns(3, 0, false);

    //ImGui::SetColumnWidth(0, 150.0f);

    ImGui::NextColumn();

    ImGui::PushFont(mDefaultLargeFontGUI);

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (sz.y/4));
    ImGui::Text("Paper Code");

    ImGui::PopFont();

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);
    ImGui::Text("Lightweight, Lightning Fast");

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10);

    ImGui::PushFont(mDefaultMidFontGUI);
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 20);
    ImGui::Text("Start");
    ImGui::PopFont();

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);
    if (ImGui_LinkButton(ICON_FA_FILE " New Project...")) {
        PaperCode::get().executeCommand(Commands::NewProject);
    }
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);
    if (ImGui_LinkButton(ICON_FA_FOLDER_OPEN " Open Project...")) {
        std::string filePath = openFileDialog("Paper Code Project (*.paper)\0*.paper\0"
                                 "All Files (*.*)\0*.*\0");
        if (!filePath.empty()) {
            PaperCode::get().executeCommand(Commands::Open, CommandData {.FilePath = filePath} );
        }
    }

    if (ImGui::BeginPopupModal("New Project", 0, 0/*ImGuiWindowFlags_NoResize*/)) {
        mNewProject.draw();
        ImGui::EndPopup();
    }

    if (ImGui::BeginPopupModal((mMsgBox.mTitle + "###Message Box").c_str(), 0, 0/*ImGuiWindowFlags_NoResize*/)) {
        mMsgBox.draw();
        ImGui::EndPopup();
    }

    ImGui::PushFont(mDefaultMidFontGUI);
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 20);
    ImGui::Text("Recent");
    ImGui::PopFont();

    ImGuiStyle& style = ImGui::GetStyle();

    float tmp = style.SeparatorTextBorderSize;
    auto padding = style.SeparatorTextPadding;

    style.SeparatorTextBorderSize = 0;
    style.SeparatorTextPadding = {0, 0};

    // TODO: Show '- No Recent Files -' text if there is none

    for(const Recent& rc : PaperCode::get().mSettings.mRecentProjects) {
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);
        if (ImGui_LinkButton(rc.nameonly, rc.filepath)) {
            PaperCode::get().openProject(rc.filepath);
        }
        ImGui::SameLine();
        ImGui::SeparatorText(rc.filepath.c_str());
    }

    style.SeparatorTextPadding = padding;
    style.SeparatorTextBorderSize = tmp;

    ImGui::NextColumn();

    //

    ImGui::Columns(1, 0, false);

    const char* copyrightNotice = "Copyright © 2024 Shah Farhad Reza";

    float win_height = ImGui::GetWindowSize().y -style.WindowPadding.y;
    float text_height = ImGui::CalcTextSize(copyrightNotice).y;

    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + style.WindowPadding.x);
    ImGui::SetCursorPosY(win_height - text_height);
    
    ImGui::Text("%s", copyrightNotice);

    ImGui::SameLine();

    const char* switchLang = "Switch Language?";

    float text_width = ImGui::CalcTextSize(switchLang).x;
    float text_width2 = ImGui::CalcTextSize("Bangla").x;

    ImGui::SetCursorPosX(ImGui::GetWindowSize().x - text_width - text_width2 - (style.WindowPadding.x*5));
    ImGui::Text("%s", switchLang);

    ImGui::SameLine();

    if (ImGui_LinkButton("Bangla", "Switch to Bangla Language?")) {
        
    }

    ImGui::End();

    ImGui::PopFont();
}

void UISystem::drawEvent() {
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL2_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    //ImGui::PushFont(mDefaultFontGUI);

    ImGui::DockSpaceOverViewport();

    if (getEditorManager().hasEditors() == false && PaperCode::get().getActiveProject() == nullptr) {
        drawUIStartPage();
    } else {
        drawUI();
    }

    //ImGui::PopFont();

    // GUI Rendering
    ImGui::Render();
    ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());

}

void UISystem::terminate() {

    getEditorManager().closeAllEditors();

    glfwDestroyWindow(mWindow);
	glfwTerminate();
}

std::string UISystem::openFileDialog(const char* filters) {
    return OpenFileDialog(mWindow, filters);
}

