#ifndef I2CLOCK_H
#define I2CLOCK_H

#include <string>
#include <fcntl.h>
#include <unistd.h>
#include <sys/file.h>
#include <QDebug>

// Lock file path template: /tmp/i2c-N.lock (N is the I2C bus number)
#define I2C_LOCK_PATH_TEMPLATE "/tmp/i2c-%d.lock"

/**
 * @brief I2C bus process lock class
 * 
 * Implements cross-process I2C bus access control using flock(), 
 * preventing multiple processes from accessing the same I2C device concurrently.
 * 
 * Design features:
 * 1. Uses file lock (flock) for cross-process mutual exclusion
 * 2. Lock file path: /tmp/i2c-N.lock (N is the I2C bus number)
 * 3. Automatic release (kernel releases lock when process exits)
 * 4. Timeout mechanism to avoid deadlock
 * 5. RAII-style management (locks on construction, unlocks on destruction)
 */
class I2CLock
{
public:
    /**
     * @brief Constructor
     * @param i2cBus I2C bus path, e.g., "/dev/i2c-1"
     * @param timeoutMs Timeout in milliseconds (0 = wait forever, default = 5000ms)
     */
    explicit I2CLock(const std::string& i2cBus, int timeoutMs = 5000)
        : m_i2cBus(i2cBus)
        , m_timeoutMs(timeoutMs)
        , m_lockFd(-1)
        , m_locked(false)
    {
        // Extract bus number from I2C path, e.g., "/dev/i2c-1" -> "1"
        size_t pos = m_i2cBus.rfind('-');
        if (pos != std::string::npos) {
            std::string busNum = m_i2cBus.substr(pos + 1);
            int busNumber = std::stoi(busNum);
            
            // Format lock file path using template
            char lockPath[256];
            snprintf(lockPath, sizeof(lockPath), I2C_LOCK_PATH_TEMPLATE, busNumber);
            m_lockFile = lockPath;
        } else {
            qWarning() << "Invalid I2C bus path:" << QString::fromStdString(m_i2cBus);
            m_lockFile = "/tmp/i2c-default.lock";
        }
    }
    
    /**
     * @brief Destructor, automatically releases the lock
     */
    ~I2CLock()
    {
        unlock();
    }
    
    /**
     * @brief Acquire the lock
     * @return true: Lock acquired successfully, false: Lock acquisition failed (timeout or other error)
     */
    bool lock()
    {
        if (m_locked) {
            return true; // Already holding the lock
        }
        
        // Open or create the lock file
        m_lockFd = ::open(m_lockFile.c_str(), O_RDWR | O_CREAT, 0666);
        if (m_lockFd < 0) {
            qWarning() << "Failed to open lock file:" << QString::fromStdString(m_lockFile) 
                         << "error:" << strerror(errno);
            return false;
        }
        
        // Implement timeout mechanism
        if (m_timeoutMs > 0) {
            // Use non-blocking mode + polling for timeout
            int elapsed = 0;
            const int retryInterval = 10; // Retry every 10ms
            
            while (elapsed < m_timeoutMs) {
                if (::flock(m_lockFd, LOCK_EX | LOCK_NB) == 0) {
                    m_locked = true;
                    return true;
                }
                
                // If lock acquisition fails, wait before retrying
                usleep(retryInterval * 1000);
                elapsed += retryInterval;
            }
            
            // Timeout
            qWarning() << "Lock timeout after" << m_timeoutMs << "ms for I2C bus:" 
                         << QString::fromStdString(m_i2cBus);
            ::close(m_lockFd);
            m_lockFd = -1;
            return false;
        } else {
            // Wait forever mode
            if (::flock(m_lockFd, LOCK_EX) == 0) {
                m_locked = true;
                return true;
            }
            
            qWarning() << "Failed to acquire lock for I2C bus:" 
                         << QString::fromStdString(m_i2cBus) << "error:" << strerror(errno);
            ::close(m_lockFd);
            m_lockFd = -1;
            return false;
        }
    }
    
    /**
     * @brief Release the lock
     */
    void unlock()
    {
        if (!m_locked || m_lockFd < 0) {
            return;
        }
        
        ::flock(m_lockFd, LOCK_UN);
        ::close(m_lockFd);
        m_lockFd = -1;
        m_locked = false;
    }
    
    /**
     * @brief Check if the lock is held
     */
    bool isLocked() const
    {
        return m_locked;
    }

private:
    // Disable copy
    I2CLock(const I2CLock&) = delete;
    I2CLock& operator=(const I2CLock&) = delete;
    
    std::string m_i2cBus;           // I2C bus path
    std::string m_lockFile;         // Lock file path
    int m_timeoutMs;                // Timeout in milliseconds
    int m_lockFd;                   // Lock file descriptor
    bool m_locked;                  // Lock status
};

/**
 * @brief I2C lock guard class (RAII)
 * 
 * Usage example:
 * ```
 * {
 *     I2CLockGuard guard("/dev/i2c-1");
 *     if (guard.isLocked()) {
 *         // Safely access I2C device
 *         readData(...);
 *     }
 * } // Automatically releases the lock
 * ```
 */
class I2CLockGuard
{
public:
    explicit I2CLockGuard(const std::string& i2cBus, int timeoutMs = 5000)
        : m_lock(i2cBus, timeoutMs)
    {
        m_lock.lock();
    }
    
    ~I2CLockGuard()
    {
        // Automatically unlocks on destruction
    }
    
    bool isLocked() const
    {
        return m_lock.isLocked();
    }

private:
    I2CLock m_lock;
};

#endif // I2CLOCK_H
