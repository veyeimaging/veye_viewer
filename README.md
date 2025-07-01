# veye_viewer

### Installation
The UI is developed using Qt5, and the video acquisition is based on the v4l2 camera demo development. It is compatible with the MV series cameras and RAW series cameras, and supports the platforms RK3566/RK3568/RK3588 series, Jetson Orin and Xavier series, raspberry4, and 5 series.

### Compilation Tutorial
The source code is built using CMake. If you need to compile the source code, you can use the following two methods for compilation. Before compiling, you need to install the basic environment and script authorization to avoid affecting the operation.

#### Basic Environment Installation
    sudo apt update
    sudo apt-get install gawk
    sudo apt-get install qtcreator
    sudo apt-get install qtbase5-dev
    sudo apt-get install qttools5-dev 
    sudo apt-get install cmake
    sudo apt-get install g++
    sudo apt-get install gdb
#### Script Authorization
    cd veye_viewer
    chmod 755 
    ./config/*.sh
#### CMake Compilation and Execution
    cd build
    cmake ..
    make
    ./start_veye_viewer.sh
#### Qt Creator Compilation
1. Launch Qt Creator: Open Qt Creator.
2. Open Project: In Qt Creator, select File > Open File or Project, then navigate to the location of your CMakeLists.txt file. Make sure to select the CMakeLists.txt file in the root directory of the project.
3. Choose Kit: Before opening the project, Qt Creator will ask you to select a Kit. A Kit refers to a set of pre-configured toolchains in the development environment, including compilers and Qt versions. If you haven't configured a Kit yet, you can click Manage Kits... to add or configure a new Kit. Typically, you need to configure at least one Kit with the correct compiler and Qt version.
4. Configure Project: After selecting a Kit, Qt Creator will try to load the project. If everything is set up correctly, it will automatically recognize it as a CMake project.
5. Build and Run: Once configured, you can click the green play button in the lower left corner to build and run your project.

#### Special Note: When compiling the source code using qtcreator, after the first compilation is complete, you need to use `./start_veye_viewer.sh` to run.
