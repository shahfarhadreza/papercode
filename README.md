# Paper Code
Tired of bloated IDEs that slow you down? Welcome to **Paper Code**, the lighteight, fast C++ IDE built for performance-obsessed developers.

![preview](screenshots/02.png)

# Focus on Performance

* **Lightweight Design:** Built with a focus on minimal resource usage, ensuring smooth operation even on older or less powerful machines.
* **Hardware-Accelerated UI:** Utilizes OpenGL for a responsive, lag-free and cross-platform user interface.

# Features:

* Code Editing:
  - Syntax highlighting for C++ code.
  - Support for indentation and code formatting.
  - Code Auto Completion (Keywords only).
  - Intellisense (Work-in progress).
* Project Management:
  - Ability to create and manage C++ projects with multiple files.
* Other Features:
  - A customizable interface to suit different preferences (Work-in progress).
  - Support for cross-platform development (Tested on Windows & Linux Debian Only) - Still work in progress.

# Download

### Paper Code
See Release - https://github.com/shahfarhadreza/papercode/releases/tag/beta-build-11pm-14-april-2024

### C++ Compiler

GCC Compilers (MinGW-w64) - https://winlibs.com/#download-release
Don't forget to add the compiler path (bin folder) to the environment variables of operating system. Otherwise Paper Code won't be able to find your GCC compiler in order to build your C++ projects.

# How to Build

**Paper Code** uses `CMake` to support cross-platform building. Install CMake before proceeding.

**Note:** If you don't use the provided installer for your platform, make sure that you add CMake's bin folder to your path.

C++ Version Required: C++23 Or Higher

The basic steps to build are:

1. **Generate**

    Navigate into the root directory (where CMakeLists.txt is), create build folder and run CMake:
   
```
mkdir build
cd build
cmake [-G generator] ..\
```

The `generator` option is the build system you'd like to use. For example: `cmake -G"Ninja" ..\`

2. **Build**

The command you'll need to run depends on the generator you chose earlier. For example: Write `ninja` and hit enter. (Make sure you are in the build folder)

3. **Run**

After the building process completes, you will find the executable in the bin folder to run. 

# Build on linux

Currently testd only on Debian testing. Ubuntu 22.04 is probably not going to work, lets wait to 
Ubuntu 24.04, hopefully this will work as well.

Install build dependencies, and build:
```
sudo apt install clang-17 clang-tools-17 build-essential ninja cmake xorg-dev
CXX=clang+-17 cmake -C cbuild -G Ninja
cmake -b cbuild
```

Note that due to heavy usage of C++20,C++23 - gcc is not alawys supported  (gcc 13 cannot compile 
this project). Clang 18 should compile, but it is not available on Debian yet. For this reason
the GNUMakefile generator will not work and you must use ninja.

# Dependencies

* GLFW
* ImGui
* STB
* Yaml-Cpp
* FontAwesome5

# Contributing

We welcome contributions from the C++ developer community!
