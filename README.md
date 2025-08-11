
# veye_viewer

### Introduction
**veye_viewer** is an open-source, Qt-based client software specifically developed for VEYE IMAGING camera modules. It features a Qt5-based user interface and uses the V4L2 interface for video capture.

### Compatibility

Compatible with both **MV series** and **RAW series** camera modules.

Supported platforms include:
- RK3566 / RK3568 / RK3588 series
- Jetson Orin and Xavier series
- Raspberry Pi 4 and 5 series

### Download and Run
You can either download the executable file from the [Releases](https://github.com/veyeimaging/veye_viewer/releases) section of this project or build it from source.

#### Run

```bash
cd veye_viewer
chmod 755 ./config/*.sh   
./start_veye_viewer.sh
```

### User Guide
*To be added.*

### Limitations
Currently supports only the **RAW8** data format.

### Build from Source

The source code is built using **CMake**. There are two main ways to compile the project. Before building, make sure to install the necessary dependencies and grant script execution permissions.

#### Install Required Dependencies

```bash
sudo apt update
sudo apt-get install gawk
sudo apt-get install qtcreator
sudo apt-get install qtbase5-dev
sudo apt-get install qttools5-dev 
sudo apt-get install cmake
sudo apt-get install g++
sudo apt-get install gdb
````

#### Build with CMake

```bash
cd build
cmake ..
make
```

#### Build with Qt Creator

1. **Launch Qt Creator**: Open Qt Creator.
2. **Open the Project**: Go to *File > Open File or Project* and navigate to the `CMakeLists.txt` file in the root directory.
3. **Select a Kit**: Qt Creator will prompt you to select a Kit (a pre-configured environment with compiler and Qt version). If no Kit is available, click *Manage Kits...* to add one.
4. **Configure the Project**: Once a Kit is selected, Qt Creator will recognize the project as a CMake project.
5. **Build**: Click the green "Run" button in the bottom left corner to build the project.


