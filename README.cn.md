[English](README.md) | [中文](README.cn.md)

# veye_viewer

### 简介
veye_viewer 是一个开源的、基于 Qt 的、为 VEYE IMAGING 相机模组专门开发的客户端软件。其 UI 采用 Qt5 实现交互，视频获取基于 V4L2 接口。

### 兼容性

兼容 MV 系列、RAW 系列和 GX 系列相机模组。

支持平台：RK3566/RK3568/RK3588 系列、Jetson Orin 和 Xavier 系列、Raspberry Pi 4/5 系列。

### 下载和运行

可以直接从本项目的 release 包中下载可执行文件，也可以选择从源码编译（见[源码编译](#源码编译)）。

#### 1. 安装依赖

项目提供 `setup.sh` 一键安装依赖。默认只安装运行程序所需的**运行时**依赖：

```bash
cd veye_viewer
chmod +x ./setup.sh
./setup.sh
```

它会安装 `gawk`、`v4l-utils`（提供 `media-ctl` / `v4l2-ctl`）以及 Qt5 运行时库，并自动给启动/探测脚本加上可执行权限，同时尝试把当前用户加入 `i2c` 组。

`setup.sh` 参数说明：

| 参数 | 说明 |
| ---- | ---- |
| （无参数） | 只安装运行时依赖（日常使用推荐） |
| `--build` | 额外安装编译工具链（Qt5 开发库、cmake、g++、make、gdb） |
| `--ide` | 额外安装 Qt Creator |
| `--no-i2c` | 跳过把当前用户加入 `i2c` 组的步骤 |
| `--help` | 查看帮助 |

> 如果只是下载现成的可执行文件，默认执行 `./setup.sh` 即可。

#### 2. 运行

```bash
./start_veye_viewer.sh
```

首次运行会自动检测平台，执行对应的 `config/probe_camera_info-*.sh` 脚本生成 `config/auto_camera_index.json`，然后启动 `veye_viewer`。

#### 使用说明
使用中请注意界面下方的日志提示。如果出现访问 `/dev/i2c-*` 没有权限的情况，请重新登录（或重启）让 `i2c` 组生效，或手动授权：

```bash
sudo chmod 777 /dev/i2c-*
```

#### 使用限制
MV 和 RAW 系列相机仅支持 RAW8 数据格式。

### 源码编译

源码采用 CMake 构建，可使用命令行或 Qt Creator 进行编译。

#### 1. 安装编译环境

在运行时依赖之外，额外安装编译工具链：

```bash
cd veye_viewer
./setup.sh --build
```

会安装：`cmake`、`g++`、`make`、`gdb`、`qtbase5-dev`、`libqt5opengl5-dev`、`qttools5-dev` / `qttools5-dev-tools`（提供 `lupdate` / `lrelease`）。如果还需要 Qt Creator，加上 `--ide`。

#### 2. CMake 编译运行

```bash
mkdir -p build
cd build
cmake ..
make
./start_veye_viewer.sh
```

> `cmake`/`make` 会把 `start_veye_viewer.sh` 和 `config/` 目录复制到 `build/` 下，因此要在 `build/` 目录里运行启动脚本。

#### 3. Qt Creator 编译

1. 启动 Qt Creator。
2. 打开项目：选择 **File > Open File or Project**，选中项目根目录下的 `CMakeLists.txt`。
3. 选择 Kit：Qt Creator 会要求选择一个 Kit（即包含编译器和 Qt 版本的一组预配置工具链）。若尚未配置，可点击 **Manage Kits...** 配置一个包含正确编译器和 Qt5 版本的 Kit。
4. 配置项目：选择 Kit 后，Qt Creator 会加载项目并自动识别为 CMake 项目。
5. 构建和运行：点击左下角的绿色运行按钮。

> **注意：** 使用 Qt Creator 编译时，首次编译完成后请通过 `./start_veye_viewer.sh`（在构建目录中）运行，以便正确加载探测脚本和配置。
