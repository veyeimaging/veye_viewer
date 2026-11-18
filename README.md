[English](README.md) | [中文](README.cn.md)

# veye_viewer

### Introduction
veye_viewer is an open-source, Qt-based client application developed specifically for VEYE IMAGING camera modules. Its user interface is built with Qt5, and video acquisition is implemented through the V4L2 interface.

#### Compatibility

Compatible with MV-series, RAW-series, and GX-series camera modules.

Supported platforms: RK3566/RK3568/RK3588 series, Jetson Orin and Xavier series, and Raspberry Pi 4/5 series.

### Download and Run

You can either download a pre-built executable from this project's releases, or compile from source (see [Build from Source](#build-from-source)).

#### 1. Install dependencies

A helper script `setup.sh` installs everything for you. By default it only installs the **runtime** dependencies needed to launch the app:

```bash
cd veye_viewer
chmod +x ./setup.sh
./setup.sh
```

This installs `gawk`, `v4l-utils` (provides `media-ctl` / `v4l2-ctl`), and the Qt5 runtime libraries, marks the start/probe scripts executable, and tries to add your user to the `i2c` group.

`setup.sh` options:

| Option | Description |
| ------ | ----------- |
| *(none)* | Install runtime dependencies only (recommended for normal use) |
| `--build` | Additionally install the compile toolchain (Qt5 dev headers, cmake, g++, make, gdb) |
| `--ide` | Additionally install Qt Creator |
| `--no-i2c` | Skip adding the current user to the `i2c` group |
| `--help` | Show usage |

> If you only downloaded a pre-built binary, the default `./setup.sh` is enough.

#### 2. Run the application

```bash
./start_veye_viewer.sh
```

The first launch automatically detects the platform and runs the matching `config/probe_camera_info-*.sh` script to generate `config/auto_camera_index.json`, then starts `veye_viewer`.

#### Usage notes
Watch the log messages at the bottom of the interface. If you see a "no permission on /dev/i2c-*" error, re-login (or reboot) so the `i2c` group takes effect, or grant access manually:

```bash
sudo chmod 777 /dev/i2c-*
```

#### Limitations
MV and RAW series cameras support the RAW8 data format only.

### Build from Source

The project is built with CMake. You can build from the command line or with Qt Creator.

#### 1. Install the build environment

Install the compile toolchain in addition to the runtime dependencies:

```bash
cd veye_viewer
./setup.sh --build
```

This installs: `cmake`, `g++`, `make`, `gdb`, `qtbase5-dev`, `libqt5opengl5-dev`, `qttools5-dev` / `qttools5-dev-tools` (for `lupdate` / `lrelease`). Add `--ide` if you want Qt Creator as well.

#### 2. CMake build and run

```bash
mkdir -p build
cd build
cmake ..
make
./start_veye_viewer.sh
```

> `cmake`/`make` copy `start_veye_viewer.sh` and the `config/` directory into `build/`, so run the start script from inside `build/`.

#### 3. Qt Creator build

1. Launch Qt Creator.
2. Open project: choose **File > Open File or Project** and select the `CMakeLists.txt` in the project root.
3. Choose a Kit: Qt Creator asks you to pick a Kit (a pre-configured toolchain with compiler and Qt version). If none exists, click **Manage Kits...** to configure one with a valid compiler and the Qt5 version.
4. Configure: once a Kit is selected, Qt Creator loads the project and recognizes it as a CMake project.
5. Build and run: click the green run button in the lower-left corner.

> **Note:** when building with Qt Creator, after the first successful build, run the app via `./start_veye_viewer.sh` (from the build directory) so the probe scripts and config are picked up correctly.
