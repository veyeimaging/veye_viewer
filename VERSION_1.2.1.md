# veye_viewer v1.2.1 版本升级说明

## 版本信息
- **版本号**: v1.2.1
- **发布日期**: 2026-11-07
- **类型**: 功能增强

---

## 新增功能

### I2C总线进程锁机制

#### 功能描述
新增跨进程的I2C总线访问锁机制，保护 `/dev/i2cN` 设备在多进程环境下的并发访问安全。

#### 技术实现
- **锁类型**: 基于 `flock()` 的文件锁
- **锁文件路径**: `/tmp/i2c-N.lock`（N为I2C总线号）
- **释放机制**: 进程退出时自动释放
- **超时保护**: 默认5秒超时，避免死锁

#### 核心组件
1. `I2CLock` 类 - 基础锁操作（加锁、解锁）
2. `I2CLockGuard` 类 - RAII风格的自动锁管理
3. 已集成到 `Camera::readData()` 和 `Camera::writeData()`

#### 使用示例
```cpp
// 方式1：自动管理（推荐）
{
    I2CLockGuard guard("/dev/i2c-1");
    if (guard.isLocked()) {
        // 安全地访问I2C设备
        readData(...);
    }
} // 自动释放

// 方式2：手动管理
I2CLock lock("/dev/i2c-1", 3000); // 3秒超时
if (lock.lock()) {
    // I2C操作
    lock.unlock();
}
```

#### 性能影响
- 单次I2C读取额外开销: **+0.2ms**
- 周期性读取(1次/秒): **< 1% CPU**
- 对现有应用影响: **可忽略**

---

## 文件变更

### 新增文件
- `I2CLock.h` - I2C锁实现头文件
- `CHANGELOG.md` - 版本更新记录
- `VERSION_1.2.1.md` - 本升级说明

### 修改文件
- `Camera.cpp` - 集成锁机制
- `CMakeLists.txt` - 版本号更新为 1.2.1

---

## 兼容性说明

✅ **完全向后兼容**
- 现有代码无需修改
- 锁机制自动生效
- API接口保持不变

---

## 故障排查

### 问题1: 加锁超时
**现象**: 日志显示 "Lock timeout after 5000ms"

**解决**:
```bash
# 查看锁文件
ls -l /tmp/i2c-*.lock

# 清理残留锁（确保无进程使用）
rm /tmp/i2c-*.lock
```

### 问题2: 多进程冲突
**现象**: 日志显示 "failed to acquire lock"

**说明**: 这是正常的互斥行为，等待其他进程释放锁即可。

---

## 升级建议

1. **所有用户**: 建议升级到v1.2.1，特别是多进程部署场景
2. **单进程用户**: 可保持当前版本，锁机制不影响单进程使用

---

## 技术支持

如有问题，请参考：
- `CHANGELOG.md` - 详细更新记录
- `README.md` - 项目说明
- GitHub Issues: https://github.com/veyeimaging/veye_viewer/issues

---

**发布团队**: veye_viewer development team  
**发布日期**: 2026-11-07
