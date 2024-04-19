#include "Stdafx.h"
#include "PaperCode.h"
#include "TextEditor.h"

void UIEditor::init() {
    mImEditor = std::make_shared<TextEditor>();
    mImEditor->SetShowWhitespaces(false);
}

void UIEditor::destroy() {
    mImEditor = nullptr;
}

int UIEditor::getLine() const { 
    return mImEditor->GetCursorPosition().getLine(); 
}

int UIEditor::getColumn() const { 
    return mImEditor->GetCursorPosition().getColumn(); 
}

int UIEditor::getTabSize() const { 
    return mImEditor->GetTabSize(); 
}

bool UIEditor::isFileOpen(const std::filesystem::path& filepath) const { 
    return mFilePath.compare(filepath) == 0; 
}

void UIEditor::updateFilePath(const std::filesystem::path& filePath) {
    mFilePath = filePath;
    mFileName = filePath.filename().string();
}

void UIEditor::openFile(const std::filesystem::path& filePath) {
    mFilePath = filePath;
    mFileName = filePath.filename().string();

    // Check if the file actually exist (user might have deleted it from outside papercode)
    if (!std::filesystem::exists(filePath)) {
        std::cout << "ERROR: Project file '" << filePath << "' doesn't exist" << std::endl;
        return;
    }

    std::string ext = filePath.extension().string();

    std::transform(ext.begin(), ext.end(), ext.begin(), 
        [](unsigned char c){ return std::tolower(c); });

    if (ext == ".cpp" || ext == ".cxx" || ext == ".cc" || ext == ".c" || ext == ".h" || ext == ".hpp") {
        auto lang = TextEditor::LanguageDefinition::CPlusPlus();
        mImEditor->SetLanguageDefinition(lang);

    } else {
        std::cout << "WARNING: Unknown file extension '" << ext << "'. Setting as C++" << std::endl;
        auto lang = TextEditor::LanguageDefinition::CPlusPlus();
        mImEditor->SetLanguageDefinition(lang);
    }

    std::ifstream t(filePath);
    if (t.good()) {
        std::string str((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
        mImEditor->SetText(str);
        t.close();

        mFirstLoaded = true;
    } else {
        std::cout << "Failed to load file '" << filePath << "'" << std::endl;
    }
}

void UIEditor::saveToFile() {
    if (mFilePath.empty())
        return;
    // Lets see if the file doesn't exist yet
    if (!std::filesystem::exists(mFilePath)) {
        std::cout << "LOG: File '" << mFilePath << "' doesn't exist. Creating one..." << std::endl;
        // The given path parent directories might not as well
        std::filesystem::path pathOnly = mFilePath.parent_path();
        bool pathExists = std::filesystem::is_directory(pathOnly);

        if (!pathExists) {
            std::cout << "LOG: Path directories also don't exist. Creating one..." << std::endl;

            if (!std::filesystem::create_directories(pathOnly)) {
                std::cout << "CRITICAL ERROR: Failed to create directory '" << pathOnly << "'" << std::endl;
            } else {
                std::cout << "LOG: Directory created " << std::endl;
            }
        }
    }

    std::ofstream t(mFilePath); 

    if (t.good()) {
        std::string text = mImEditor->GetText();
        t << text;
        t.close();

        setModified(false);
        std::cout << "LOG: File saved successfuly '" << mFilePath << "'" << std::endl;
    } else {
        std::cout << "ERROR: Failed to save file '" << mFilePath << "'" << std::endl;
    }
}


