#include <string>
#include <GLFW/glfw3.h>

#include <pfd/portable-file-dialogs.h>

// TODO: Multiple files open/add
std::string OpenFileDialog(GLFWwindow* window, std::vector<std::string> const &filters) {
    // File open
    auto f = pfd::open_file("Open", pfd::path::home(), filters);
    if (!f.result().empty()) {
        return f.result()[0];
    }
    return std::string();
}

std::string SaveFileDialog(GLFWwindow* window, const std::string& filepath, std::vector<std::string> const &filters) {
    auto f = pfd::save_file("Save as", filepath, filters);
    return f.result();
}

