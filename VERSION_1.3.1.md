# veye_viewer v1.3.1 版本升级说明

## 版本信息
- **版本号**: v1.3.1
- **发布日期**: 2026-11-12
- **类型**: 功能增强 + 性能优化 + 健壮性修复
- **上一发布版本**: v1.2.1（v1.3.0 只起草未发布，其功能并入本版）

---

## 新增功能

### 1. 多相机同时预览（最多 4 路）

#### 功能描述
从"全局单实例单相机"改为通道化模型：4 个通道各自独立持有 Camera、采集线程、帧队列、渲染视图与参数快照，可同时预览 4 台相机。

#### 技术实现
- `CameraChannel m_channels[MAX_CHANNELS]`（`MAX_CHANNELS = 4`），每通道独立的 `queueSii` / `mtxPix` / `Camera *cam`
- 三种布局：**单画面(1x1) ↔ 四画面(2x2)** 由 `applyViewLayout()` 动态增删 grid 项实现；**全屏**支持单格全屏与四画面全屏，双击某格进入、ESC 或再双击退出
- **激活通道机制**：单击任意可见格（含空格）即可激活，右侧参数面板与"开始采集/显示"只作用于激活格；配合"先点目标格、再选设备"即可把新相机放进指定格
- 激活格以 3px 黑色边框高亮，其余格 1px 灰框
- 切换通道时主动把当前设备类型/平台下发给所有已打开的对话框，对话框不再沿用构造时的设备快照（修复 GX 相机一直走 MV 分支、面板与寄存器命令格式全错的问题）

#### 使用提示
- 四画面下每个格子需要**各自点击"开始采集"**，与原有单格工作流一致
- 设备类型未知时该通道不渲染并留一条日志，避免按错误格式解出花屏

---

### 2. GX 系列预览链路：full range 原样显示

#### 功能描述
GX 相机直接从 sensor 出 **full range** 的 UYVY（0~255 全部有效），旧版按视频常用的 limited range 解释，导致亮度被压平、高光被吃。

#### 修改点
- 不再把 `Y` 钳到 16~235、不再把 `U/V` 钳到 16~240
- 不再做 limited→full 的 `*255/224` 拉伸
- 只保留 8bit RGB 落地时无法避免的 0~255 饱和
- 公式取 full-range BT.601：`R = Y + 1.402·(V-128)`，`G = Y - 0.344136·(U-128) - 0.714136·(V-128)`，`B = Y + 1.772·(U-128)`

#### 实测依据（真机 GX 相机 1920x1080 帧）
- 原始数据 Y 最大值 254，**77.35% 的像素 Y > 235**（旧版把这部分全部压平成同一档亮度）
- 新旧实现对比：100% 像素发生变化，单通道最大差 39 LSB，平均差 30.43 LSB

---

## 性能优化

### 1. UYVY→RGB 转换重写：提速约 2.8 倍

新增独立模块 `UyvyConvert.h` / `UyvyConvert.cpp`：

| 手段 | 说明 |
|---|---|
| 查表消除法 | 色度三项只与 0~255 的原始字节有关，预生成 Q4 定点（1/16 LSB）查表，热循环里**一次除法都没有**（旧实现每像素对做 6 次带符号除法） |
| NEON 向量化 | 每轮 8 像素；宽度不是 8 倍数时尾巴走标量。ARMv8 之外自动退回纯标量路径 |
| 输出 RGB32 | 旧版输出 `Format_RGB888`，`QPixmap::fromImage()` 要再逐像素展开成 32 位；直接出 `Format_RGB32` 后只剩拷行，且消除三字节非对齐写 |
| 去掉内层分支 | 旧实现每像素 `if(...) qWarning()` 判定，阻断自动向量化 |

**实测耗时（RK3588S，1920x1080 单帧）**

| 实现 | 大核 A76 | 小核 A55 |
|---|---|---|
| 改造前 | 20.1 ms | 36.1 ms |
| 改造后 | **7.2 ~ 7.8 ms** | 13.0 ms |

**精度验证**（保证"快了但颜色没变"不发生）
- NEON 与标量路径输出 **8294400 字节逐字节相同**
- 与 double 浮点 full-range 参考比对：207.36 万采样**最大偏差 1 LSB，超过 1 LSB 的像素 0 个**
- 分行区间调用与整帧一次调用**完全一致**；奇数宽 1917/1918/5 同样 1 LSB，ASAN 无越界
- app 侧 `QImage` + `bits()/bytesPerLine()` 接线与直接调用产物逐字节相同

### 2. 帧处理线程改为每通道一根（四画面不再串行）

#### 问题
原先 4 个通道共用**一根** `workThread()`，在 `for` 循环里逐路取帧转换。转换是纯算力活，四路就是四倍时间，与板子有多少核无关：
- 大核：4 × 9.4 = 38 ms（已超 33ms 的 30fps 预算）
- 全落小核：4 × 18.4 = 74 ms（≈ 13 fps，实测四画面 ~10fps 与此吻合）

#### 改动
- `workThread()` → `workThread(int ch)`，`MainWidget` 起 `MAX_CHANNELS` 根线程，每根只碰自己通道的队列/锁/Camera
- 跨线程标志改原子：`m_setFps`、`m_scale`（原为普通变量，GUI 线程写、帧线程读）
- `CameraChannel::gvSize` 读写统一在 `mtxPix` 保护下（新增 `setGvSize()`）；帧线程在**同一把锁里**同时取帧和尺寸，杜绝 resize 期间读到"只写了一半的宽高"
- `~MainWidget()` 逐通道 `join()` 之后再拆通道

#### 实测（整链：转换 + `QPixmap::fromImage` + `scaled(960,540)`，每线程独立缓冲）

| 绑核 | 线程数 | 单帧转换 | 整链每帧 | 吞吐 |
|---|---|---|---|---|
| 大核 4-7 | 1 | 7.52 ms | 9.7 ms | 103 fps |
| 大核 4-7 | 2 | 7.59 ms | 9.4 ms | 213 fps |
| 大核 4-7 | **4** | **7.60 ms** | **9.4 ms** | **426 fps** |
| 小核 0-3 | **4** | 13.15 ms | 18.4 ms | 218 fps |

- 吞吐线性增长、单帧耗时不上涨 → **瓶颈是算力，不是内存带宽**
- 最坏情况（4 根线程全被调度到小核）每路 18.4 ms，仍在 33 ms 预算内，**30fps 达标且不依赖绑核**
- 附带收益：4 路 pixbuf 不再在同一节拍里成"爆发"投给 GUI 线程，改为各自错相投递，画面更平

---

## 安全与健壮性修复

- **外部命令执行**：清除所有 `system()` / `/bin/sh` 调用路径，命令由带引号感知的分词器拆分后交给 `QProcess` 直接执行，关闭 shell 注入面，并不再写固定名临时文件
- **栈溢出**：UI 输入不再先 `sprintf()` 进 256 字节栈缓冲再交给 shell
- **线程生命周期**：采集/保存/i2c 命令/帧渲染线程改用 `std::atomic<bool>` 通知并在析构时 `join()`，不再 detach 后不管（修复关闭设备或退出时的 use-after-free）
- **泄漏**：`Capture` 对象会被释放而不再作为 Qt 子对象堆积；停止采集会排空待保存队列与未消费的帧队列
- **输入校验**：ROI/帧率编辑做范围检查并带整数校验器；JSON 里读到的设备节点与 entity 名先校验再作为命令参数；探测脚本输出 JSON 时对内核提供的字符串做转义
- **I2C 锁**：总线号改为手工解析而非 `std::stoi()`（后者可能从 Qt 槽里抛出并终止进程）；锁 fd 以 `O_CLOEXEC` 打开，子进程不再继承并占住总线；`/tmp` 可预测路径加 `O_NOFOLLOW` 保护；`flock()` 轮询对 `EINTR` 重试
- **缓冲区越界**：出队的 V4L2 buffer index 先做范围校验再用于索引 mmap 缓冲表
- **忙等**：渲染循环按出帧节拍休眠而非 1us 忙等（四路时不再白白吃掉一颗核）；GUI 主线程优先级由 TimeCritical 降为 High
- **未知平台 / 未知设备类型**成为显式状态：给出明确提示，而不是把配置目录当脚本执行或复用上一通道的相机类型

---

## 构建与发布

- 版本号升至 **1.3.1**；`Version.h` 只生成到构建目录，不再持续弄脏源码树
- 翻译源码只在 `config/` 下维护一份（仓库根目录的重复副本已删除）；`.qm` 构建时用 `lrelease` 重新生成，`lupdate` 移到显式的 `update_translations` 目标，构建不再改写源码树
- 移除 `-ffast-math`（对本应用的数值精度有风险且无可测收益）
- 移除 `QT_NO_DEBUG_OUTPUT` / `QT_NO_WARNING_OUTPUT`（此前会把 `qDebug()`/`qWarning()` 全部编译掉，日志文件几乎为空）
- 第三方 `json/` 源码不带警告编译，保证项目自身的警告计数有意义；源文件收集改用 `CONFIGURE_DEPENDS` 并排除构建目录；补上 `config/` 的安装规则
- 清理编译警告：sign-compare、未用变量、implicit fall-through、忽略的限定符、成员初始化顺序、弃用重载、trigraph 序列。**`-Wall` 构建项目自身源码 0 警告**

---

## 文件变更

### 新增文件
- `UyvyConvert.h` / `UyvyConvert.cpp` - UYVY→RGB32 转换模块（full range、Q4 定点查表、NEON + 标量回退）
- `CHANGELOG.md` 的 v1.3.1 段
- `VERSION_1.3.1.md` - 本升级说明

### 主要修改文件
- `MainWidget.cpp` / `MainWidget.h` - 通道化模型、布局/全屏、每通道帧处理线程、`uyvyToQImage()` 改为校验 + 调 `UyvyConvert::fillRgb32()`
- `Camera.cpp` / `Capture.cpp` / `CaptureSPlane.cpp` / `CaptureMPlane.cpp` - 每通道实例化、buffer index 校验、线程退出通知、去 shell 调用
- `CommStuct.h` - `MAX_CHANNELS`、通道相关结构
- 各对话框（`ImgAttrDlg` / `ExpGainDlg` / `TriggerDlg` / `RegisterDlg` / `CurAttrDlg` / `WhiteBlanceDlg` / `ImgProcDlg`）- 接收当前通道设备类型/平台
- `QxHelp.*` - 命令分词与 `QProcess` 执行、字符串校验
- `I2CLock.h` - 锁获取健壮性（`O_CLOEXEC` / `O_NOFOLLOW` / `EINTR`）
- `CMakeLists.txt` - 版本号、翻译流程、源文件收集、警告清理
- `config/probe_camera_info-{rk,rpi5,jetson}.sh` - 输出 JSON 的字符串转义与探测修正
- `.gitignore` - 忽略源码树里的 `Version.h` 生成物与根目录旧翻译副本

### 删除文件
- `Version.h`（改为构建目录生成）
- 仓库根目录的 `veye_viewer_{en_US,zh_CN}.{ts,qm}`（只保留 `config/` 下一份）

---

## 兼容性说明

✅ **配置文件布局不变**，已有 `config/syscfg.json` 可直接沿用

⚠️ **行为差异**
- 四画面需要**逐格点击"开始采集"**，不再是一键全起
- GX 预览亮度/对比与 v1.2.x 明显不同（这是修正：旧版把 full range 当 limited range 处理，77% 的像素亮度被压平）
- 平台相关：`UyvyConvert` 在 ARMv8 上走 NEON，其他架构自动退回标量路径，两者输出逐字节一致

---

## 故障排查

### 问题 1：升级后界面没变化 / 新功能不出现
板上通常存在**两份程序**：源码构建产物 `~/veye_viewer/build/veye_viewer` 与独立部署目录（如 `~/veye_viewer_rockchip_bin/veye_viewer`）。部署目录里若是旧二进制，改动不会生效。

```bash
pgrep -af veye_viewer                      # 找到实际运行的 PID
readlink -f /proc/<pid>/exe                # 看真实可执行文件路径
strings -a <二进制> | grep pushButtonFullScreen   # 确认是否含新代码
```
升级部署目录时请把新构建的 `veye_viewer` 与 `config/`（含 `.qm`、`*_register.json`、`probe_*.sh`）一起替换过去。

### 问题 2：多相机并发时 i2c 报加锁超时
```bash
ls -l /tmp/i2c-*.lock      # 查看锁文件
```
日志出现 "failed to acquire lock" 属于正常互斥，等待其他进程释放即可。

### 问题 3：非 root 用户下线程优先级不生效
`CaptureSPlane::run()` 里的 `setPriority(TimeCriticalPriority)` 在没有 `CAP_SYS_NICE`（`RLIMIT_RTPRIO=0`）时是空转，Qt 不会真的提升。本版已把 GUI 主线程降为 High，避免给了 RT 权限后主线程反而饿死采集线程。四画面达标不再依赖优先级或绑核。

### 问题 4：新增中文界面串没有英文
英文译文在 `config/veye_viewer_en_US.ts` 中手工维护，构建时 `lrelease` 生成到 `config/*.qm`（并复制到构建目录的 `config/`）。补完译文后可用 `cmake --build <build> --target update_translations` 检查是否有遗漏条目。

---

## 升级建议

1. **使用 GX 系列相机的用户**：建议升级。预览量程修正 + 转换提速约 2.8 倍。
2. **需要多相机同时预览的用户**：必须升级到本版才有四画面/全屏能力。
3. **四画面卡顿的场景**：本版把帧处理改为每通道一根线程，四路 1080p 最坏情况（全在小核）仍为每路 18.4 ms，可跑满 30fps。
4. **只用单路 MV 系列相机的用户**：可平滑升级，交互与配置均未破坏性改动。

---

## 本版验证清单（Orange Pi CM5 / RK3588S，内核 5.10.160）

- [x] 全新目录 configure + `-Wall` 构建：**0 error / 0 warning**
- [x] NEON 与标量路径逐字节一致；与 double 浮点参考最大偏差 1 LSB
- [x] 分行区间调用与整帧调用一致；奇数宽度 ASAN 无越界
- [x] app 的 `QImage` 接线产物与直接调用逐字节相同
- [x] 真机抓帧证实 full range（Y max=254，77.35% 像素 Y>235）
- [x] 4 线程并发吞吐线性（426 fps），内存带宽未饱和
- [x] 运行时烟测：进程内出现 4 根空闲帧处理线程（各约 0.8% CPU），应用正常启动无崩溃
- [ ] 待办：真机 GUI 目视确认四画面帧率与关窗正常退出（需人工点击"开始采集"）
- [ ] 待办：4 路真机并发采集 / i2c 总线竞争需多相机实机验证

---

## 技术支持

如有问题，请参考：
- `CHANGELOG.md` - 详细更新记录
- `README.md` / `README.cn.md` - 项目说明
- GitHub Issues: https://github.com/veyeimaging/veye_viewer/issues

---

**发布团队**: veye_viewer development team  
**发布日期**: 2026-11-12
