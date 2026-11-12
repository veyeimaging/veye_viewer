# veye_viewer Version Change Log

## v1.3.1 (2026-11-12)

> Version note: v1.3.0 was never released and never tagged; the multi-camera work drafted under it ships in this release together with the full-range and performance changes.

### New Features

#### Multi-camera simultaneous preview (up to 4 channels)
- Each view cell owns an independent channel: its own `Camera`, frame queue, QGraphicsScene/view and parameter snapshot
- New Single View / Quad View / Fullscreen layout buttons (localized in both Chinese and English)
- Quad View opens by clicking Start Capture in each cell; Fullscreen restores the layout and maximized state it came from
- Channel switching now pushes the current device type/platform to every open dialog, so dialogs no longer keep the device configuration captured at construction time

### Performance

#### GX preview path: UYVY -> RGB conversion rewritten (about 2.8x faster) and shown in true full range

**Background**:
- The whole conversion happened in a single per-pixel loop in `MainWidget::uyvyToQImage()`: six signed divisions per pixel pair, `qBound()` clamping and a `qWarning()` branch inside the innermost loop, which also blocked auto-vectorization
- Measured on RK3588S: 20.1 ms/frame on a big core, 36.1 ms/frame on an LITTLE core. Quad View serializes four channels through one worker thread, so ~92 ms/frame gave roughly 10 fps against a 33 ms budget

**Changes**:
- New `UyvyConvert` module (`UyvyConvert.h` / `UyvyConvert.cpp`): all chroma terms are precomputed as Q4 fixed-point lookup tables, so the hot loop contains no division at all; an NEON path processes 8 pixels per iteration and falls back to the scalar path for the row tail
- Output switched from `Format_RGB888` to `Format_RGB32`, which removes the per-pixel 24-to-32-bit expansion inside `QPixmap::fromImage()` and the unaligned 3-byte writes in the conversion itself
- GX sensors emit full-range UYVY, so the limited-range interpretation is gone: `Y` is no longer clamped to 16-235, `U`/`V` are no longer clamped to 16-240 and the `*255/224` stretch is dropped. Preview now shows what the camera actually outputs
- `fillRgb32()` takes a row range and indexes rows by absolute line number, which is the hook for converting a single frame on several threads later

**Verification** (real frames captured from a GX camera):
- NEON and scalar results are byte-identical over the full 8294400-byte frame; both match a double-precision full-range reference within 1 LSB on 2073600 sampled pixels
- Split-row calls reproduce the whole-frame call exactly; odd widths (1917/1918/5) are handled by the scalar tail without overrun (ASAN)
- The `QImage` + `bits()` / `bytesPerLine()` wiring used by the app produces byte-identical output to the direct call
- 7.2-7.8 ms/frame at 1920x1080, versus 20.1 ms before; 77% of pixels had their luma flattened by the old limited-range clamping

#### Frame processing: one worker thread per camera channel (Quad View is no longer serialized)

**Background**:
- All four channels shared a single `workThread()`, which popped one frame per channel in a `for` loop. Conversion is pure compute, so four channels cost 4x the time of one, no matter how many cores the board has

**Changes**:
- `workThread()` became `workThread(int ch)` and `MainWidget` now runs `MAX_CHANNELS` threads, one per channel; each only touches its own `queueSii` / `mtxPix` / `Camera`, so the four conversions happen at the same time
- Cross-thread control flags are now atomic: `m_setFps` and `m_scale` were plain variables read by the worker and written by the GUI thread
- `CameraChannel::gvSize` is now read and written under `mtxPix` (new `setGvSize()`); the worker takes the frame and the view size in one lock so it can no longer read a half-written width/height during a resize
- All worker threads are joined in `~MainWidget()` before the channels are torn down
- Side benefit: the four pixmaps are no longer delivered as a burst on one shared timer tick, so they are phased independently

**Verification** (full app chain per thread: `fillRgb32` + `QPixmap::fromImage` + `scaled`):
- 1 thread: 7.52 ms conversion / 9.7 ms per frame = 103 fps
- 2 threads: 7.59 ms conversion / 9.4 ms per frame = 213 fps
- 4 threads: 7.60 ms conversion / 9.4 ms per frame = 426 fps
- Throughput scales linearly and per-frame cost does not grow, so the limit is compute, not memory bandwidth
- Worst case of landing on four A55 cores instead of A76: 13.15 ms conversion / 18.4 ms per frame for 4 threads, still under the 33 ms budget for 30 fps
- Before this change the same four channels cost 4 x 9.4 = 38 ms (big cores) or 4 x 18.4 = 74 ms (little cores), which is what capped Quad View near 10 fps
- Startup check on the board: process now shows 4 idle worker threads at 0.8% CPU each instead of one, and the app runs normally

### Security & Robustness Fixes

- **External command execution**: removed every `system()` / `/bin/sh` path. Commands are now tokenized by a quote-aware splitter and executed directly with `QProcess`, which closes the shell-injection surface and stops writing fixed-name temporary files
- **Stack overflow**: UI input no longer goes through `sprintf()` into a 256-byte stack buffer before reaching a shell
- **Thread lifetime**: capture / save / I2C-command / frame-render threads are now signaled with `std::atomic<bool>` and `join()`ed on destruction instead of being left detached, fixing use-after-free when closing a device or quitting the app
- **Leaks**: `Capture` objects are released instead of accumulating as Qt children; stopping capture drains the pending save queue and the unconsumed frame queue
- **Input validation**: ROI/frame-rate edits are range-checked and the ROI fields carry an integer validator; device nodes and entity names read from JSON are validated before being used as command arguments; probe scripts escape kernel-provided strings when emitting JSON
- **I2C lock**: bus number is parsed by hand instead of `std::stoi()` (which could throw out of a Qt slot and terminate the app), the lock fd is opened with `O_CLOEXEC` so child processes can no longer inherit and hold the bus, `O_NOFOLLOW` guards the predictable `/tmp` path, and `flock()` polling retries on `EINTR`
- **Buffer bounds**: dequeued V4L2 buffer index is range-checked before it is used to index the mmap buffer table
- **Busy waiting**: the render loop sleeps against a frame-time budget instead of spinning, and the GUI thread priority was lowered from TimeCritical to High
- **Unknown platform / unknown device type** are now explicit states that get a clear message instead of silently executing the config directory as a script or reusing the previous channel's camera type

### Build & Release

- Version bumped to 1.3.1; `Version.h` is now generated into the build directory only, so it no longer keeps dirtying the source tree
- Translation sources live in `config/` only (the duplicated copies in the repository root were removed). `.qm` files are regenerated at build time with `lrelease`; `lupdate` moved to an explicit `update_translations` target so builds no longer rewrite the source tree
- Dropped `-ffast-math` (relaxed IEEE semantics for no measurable gain in this application)
- Dropped `QT_NO_DEBUG_OUTPUT` / `QT_NO_WARNING_OUTPUT`, which compiled all `qDebug()`/`qWarning()` output away and left the log file essentially empty
- Third-party `json/` sources are built without warnings so the project's own warning count stays meaningful; the source glob now uses `CONFIGURE_DEPENDS` and excludes the build directory, and install rules for `config/` were added
- Cleaned up compiler warnings: sign-compare, unused variables, implicit fall-through, ignored qualifiers, member-initializer order, deprecated overloads, trigraph sequences. A `-Wall` build of the project's own sources is now warning-free

### Compatibility

- Configuration file layout is unchanged; existing `config/syscfg.json` keeps working
- Quad View requires each camera to be started individually, matching the previous per-cell workflow

---

## v1.2.1 (2026-11-07)

### New Features

#### I2C Bus Process Lock Mechanism

**Background**:
- In multi-process environments, multiple processes may access `/dev/i2cN` devices simultaneously
- The original short-connection mode (reopening device for each read/write) lacked concurrent access protection
- Could lead to data races and device access conflicts

**Solution**:
- Implemented cross-process file lock mechanism based on `flock()`
- Lock file path: `/tmp/i2c-N.lock` (N is the I2C bus number)
- Supports automatic release (kernel automatically cleans up when process exits)
- Provides timeout mechanism to avoid deadlock

**Core Components**:
1. `I2CLock` class - Basic lock operations
2. `I2CLockGuard` class - RAII-style lock guard
3. Integrated into `Camera::readData()` and `Camera::writeData()`

**Usage**:
```cpp
// Automatic locking (already integrated)
int Camera::readData(StI2CCmd &cmd)
{
    I2CLockGuard lockGuard(cmd.i2c.toStdString());
    if (!lockGuard.isLocked()) {
        return QX_FAILURE;
    }
    // I2C read/write operations...
}

// Manual batch operation optimization
{
    I2CLockGuard guard("/dev/i2c-1");
    if (guard.isLocked()) {
        // Complete multiple operations under a single lock
        for (int i = 0; i < 10; i++) {
            readRegister(0x100 + i);
        }
    }
}
```

**Performance Impact**:
- Additional overhead per I2C read: ~+0.2ms
- Periodic reading (once per second): < 1% CPU usage
- Negligible impact on existing application scenarios

**Important Notes**:
- Avoid nested locking (locking the same bus multiple times in the same thread will cause deadlock)
- Prefer using `I2CLockGuard` for automatic lock lifecycle management
- Always check `isLocked()` status and implement proper error handling

### Technical Improvements

1. **Cross-process protection**: Process mutual exclusion based on `flock()`
2. **Automatic release**: Kernel automatically releases lock when process exits, preventing deadlock
3. **Timeout mechanism**: Default 5-second timeout, customizable
4. **RAII management**: Use `I2CLockGuard` for automatic lock lifecycle management
5. **Batch optimization**: Support completing multiple operations under a single lock session

### File Changes

**New Files**:
- `I2CLock.h` - I2C lock implementation header

**Modified Files**:
- `Camera.cpp` - Integrated lock mechanism into I2C read/write functions
- `CMakeLists.txt` - Maintained original build configuration

### Compatibility

- ✅ Fully backward compatible
- ✅ No existing code modifications required
- ✅ Lock mechanism automatically takes effect

### Troubleshooting

**Issue 1: Lock timeout**
```bash
# Check lock files
ls -l /tmp/i2c-*.lock

# Remove stale lock files (ensure no process is using them)
rm /tmp/i2c-*.lock
```

**Issue 2: Multi-process contention**
- This is normal mutual exclusion behavior; wait for other processes to release the lock
- Optimize code to reduce I2C access frequency

---

## v1.2.0 (2026-03)

### Main Features
- Multi-platform camera configuration support (RaspberryPi5, Rockchip, Jetson)
- Periodic camera parameter reading (once per second)
- I2C register read/write operations support
- Integrated ROI and frame rate settings functionality

---

**Document Version**: v1.1  
**Last Updated**: 2026-11-12  
**Maintained by**: veye_viewer development team
