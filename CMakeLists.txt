#
# CMakeLists.txt
# Created by Shah Farhad Reza on March 30, 2024
#
cmake_minimum_required( VERSION 3.28 )
project( papercode VERSION 0.0.1 )

set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin) 

# Enable C++23 standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# OpenGL and GLFW
find_package( OpenGL REQUIRED )

add_library(core)

target_sources(core
    PUBLIC

    FILE_SET CXX_MODULES FILES
    src/core/SubProcess.cppm
    src/core/Project.cppm
    src/core/Settings.cppm
    src/core/LogSystem.cppm
    src/core/BuildOption.cppm
    src/core/Manager.cppm
    )

set( SRCS 
    src/PaperCode.cpp
    src/PaperCode.h
    src/Stdafx.h
    src/Build.cpp
    src/UIEditor.cpp
    src/UIEditorManager.cpp
    src/UIExplorer.cpp 
    src/UIProperties.cpp 
    src/UIExplorer.cpp 
    src/UINewProject.cpp 
    src/UIPreference.cpp
    src/UISystem.cpp
    src/UIMessageBox.cpp
    src/GLFWHelper.cpp
    src/ImGuiHelper.cpp
    src/TextEditor.cpp
    src/platform/FileDialog.cpp 
    src/platform/glad/glad.c
    src/platform/glad/glad.h
    vendors/imgui/imgui.cpp
    vendors/imgui/imgui_draw.cpp
    vendors/imgui/imgui_tables.cpp
    vendors/imgui/imgui_widgets.cpp
    vendors/imgui/backends/imgui_impl_glfw.cpp
    vendors/imgui/backends/imgui_impl_opengl2.cpp
    src/icon.rc
    )

add_executable( papercode ${SRCS} )

if (WIN32)
target_compile_definitions(papercode PUBLIC WIN32)
target_compile_definitions(core PUBLIC WIN32)
endif()

add_subdirectory(vendors/glfw)
add_subdirectory(vendors/yaml-cpp)

target_include_directories(core PUBLIC vendors/yaml-cpp/include)
target_link_libraries( core yaml-cpp )

target_include_directories(papercode PUBLIC src)
target_include_directories(papercode PUBLIC src/platform)
target_include_directories(papercode PUBLIC vendors)
target_include_directories(papercode PUBLIC vendors/imgui)

target_link_libraries( papercode ${OPENGL_LIBRARIES} )
target_link_libraries( papercode glfw )
target_link_libraries( papercode core)

if (WIN32)
target_link_options( papercode PUBLIC "-Wl,--subsystem,windows" )
endif()



