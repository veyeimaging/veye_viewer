# veye_viewer

### 介绍
UI采用Qt5进行交换，视频获取基于v4l2开发相机配套demo，兼容MV 系列相机和 RAW 系列相机，支持平台RK3566/RK3568/RK3588 系列、Jetson Orin 和 Xavier 系列、raspberry4、5 系列

### 编译教程

源码采用CMake进行构建，如果需要编译源码，可以采用以下两种方式进行编译，在编译之前需要安装基础环境和脚本授权，以免影响运行。
#### 基础环境安装 
    sudo apt update
    sudo apt-get install gawk
    sudo apt-get install qtcreator
    sudo apt-get install qtbase5-dev
    sudo apt-get install qttools5-dev 
    sudo apt-get install cmake
    sudo apt-get install g++
    sudo apt-get install gdb
#### 脚本授权 
    cd veye_viewer
    chmod 755 ./config/*.sh   

#### CMake编译运行   
    cd build
    cmake ..
    make
    ./start_veye_viewer.sh
#### qtcreator编译
1. 启动 Qt Creator：打开 Qt Creator。
2. 打开项目：在 Qt Creator 中，选择 File > Open File or Project，然后浏览到你的 CMakeLists.txt 文件的位置。确保选择的是项目根目录下的 CMakeLists.txt 文件。
3. 选择 Kit：在打开项目之前，Qt Creator 会要求你选择一个 Kit。Kit 是指开发环境中预配置的一组工具链，包括编译器和 Qt 版本。如果你还没有配置 Kit，你可以点击 Manage Kits... 来添加或配置新的 Kit。通常，你需要至少配置一个包含正确编译器和 Qt 版本的 Kit。
4. 配置项目：选择 Kit 后，Qt Creator 将尝试加载项目。如果一切设置正确，它将自动识别为 CMake 项目
5. 构建和运行：配置完成后，你可以点击左下角的绿色播放按钮来构建并运行你的项目。

##### 特别说明：采用qtcreator编译源码，第一次编译完成后，需要使用./start_veye_viewer.sh运行。