# veye_viewer Version Change Log

## v1.3.0 (2026-11-12)

### New Features

#### Multi-camera simultaneous preview (up to 4 channels)
- Each view cell owns an independent channel: its own `Camera`, frame queue, QGraphicsScene/view and parameter snapshot
- New Single View / Quad View / Fullscreen layout buttons (localized in both Chinese and English)
- Quad View opens by clicking Start Capture in each cell; Fullscreen restores the layout and maximized state it came from
- Channel switching now pushes the current device type/platform to every open dialog, so dialogs no longer keep the device configuration captured at construction time

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

- Version bumped to 1.3.0; `Version.h` is now generated into the build directory only, so it no longer keeps dirtying the source tree
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
