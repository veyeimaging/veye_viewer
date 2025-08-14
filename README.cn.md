# veye_viewer

### 简介
veye_viewer是一个开源的，基于QT的，为VEYE IMAGING的相机模组而专门开发的客户端软件。其UI采用Qt5进行交互，视频获取基于v4l2接口。

### 兼容性

兼容MV 系列相机和 RAW 系列相机模组。

支持平台RK3566/RK3568/RK3588 系列、Jetson Orin 和 Xavier 系列、raspberry4、5 系列。

### 下载和运行

可以直接从本项目的release包中下载可执行文件，也可以选择从源码开始编译。

#### 安装依赖项

```bash
    sudo apt update
    sudo apt-get install gawk
```

#### 运行

```bash
    cd veye_viewer
    chmod 755 ./config/*.sh   
    ./start_veye_viewer.sh
```

### 使用说明
*To be added.*

### 使用限制
当前仅支持RAW8数据格式。

### 源码编译

源码采用CMake进行构建，如果需要编译源码，可以采用以下两种方式进行编译，在编译之前需要安装基础环境和脚本授权，以免影响运行。

#### 基础环境安装 

```bash
    sudo apt update
    sudo apt-get install gawk
    sudo apt-get install qtcreator
    sudo apt-get install qtbase5-dev
    sudo apt-get install qttools5-dev 
    sudo apt-get install cmake
    sudo apt-get install g++
    sudo apt-get install gdb
```

#### CMake编译运行

```bash
    mkdir build
    cd build
    cmake ..
    make
```

#### qtcreator编译

1. 启动 Qt Creator：打开 Qt Creator。
2. 打开项目：在 Qt Creator 中，选择 File > Open File or Project，然后浏览到你的 CMakeLists.txt 文件的位置。确保选择的是项目根目录下的 CMakeLists.txt 文件。
3. 选择 Kit：在打开项目之前，Qt Creator 会要求你选择一个 Kit。Kit 是指开发环境中预配置的一组工具链，包括编译器和 Qt 版本。如果你还没有配置 Kit，你可以点击 Manage Kits... 来添加或配置新的 Kit。通常，你需要至少配置一个包含正确编译器和 Qt 版本的 Kit。
4. 配置项目：选择 Kit 后，Qt Creator 将尝试加载项目。如果一切设置正确，它将自动识别为 CMake 项目
5. 构建和运行：配置完成后，你可以点击左下角的绿色播放按钮来构建并运行你的项目。

#### 源码编译完成之后的运行

```bash
	cd veye_viewer/build
	chmod 755 ./config/*.sh   
    ./start_veye_viewer.sh
```
