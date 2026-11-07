# veye_viewer Version Change Log

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

**Document Version**: v1.0  
**Last Updated**: 2026-11-07  
**Maintained by**: veye_viewer development team
