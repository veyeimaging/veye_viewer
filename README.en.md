# veye_viewer

### Introduction
veye_viewer is an open-source, Qt-based client application specifically developed for VEYE IMAGING camera modules. Its user interface is built with Qt5, and video acquisition is implemented via the V4L2 interface.

#### Compatibility

Compatible with MV-series, RAW-series, and GX-series camera modules.

Supported platforms include RK3566/RK3568/RK3588 series, Jetson Orin and Xavier series, and Raspberry Pi 4/5 series.

#### Download and Execution

You can either download the executable directly from the release packages of this project or choose to compile it from source code.

#### Install Dependencies

```bash
sudo apt update
sudo apt-get install gawk
```
#### Run the Application

```bash
cd veye_viewer
chmod +x ./
chmod 755 ./config/.sh
./start_veye_viewer.sh
```
#### Usage Instructions
During use, please pay attention to the log messages displayed at the bottom of the interface. If you encounter permission issues accessing i2c-num, manually grant permissions as follows:

```bash
sudo chmod 777 /dev/i2c-*
```

#### Usage Limitations
For MV and RAW series cameras, only the RAW8 data format is supported.

### Compilation Tutorial
The source code is built using CMake. If you need to compile the source code, you can use the following two methods for compilation. Before compiling, you need to install the basic environment and script authorization to avoid affecting the operation.

#### Basic Environment Installation
    sudo apt update
    sudo apt-get install gawk
    sudo apt-get install qtcreator
    sudo apt-get install qtbase5-dev
    sudo apt-get install qttools5-dev 
    sudo apt-get install libqt5opengl5-dev
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
