#pragma once

#include <Arduino.h>
#include "../Parallel/Port.h"
#include "../Parallel/HardwareFlowControl.h"
#include "../Common/Types.h"
#include "../Common/Config.h"
#include "../Common/ServiceLocator.h"


namespace DeviceBridge::Components {

// Forward declaration for callback
class FileSystemManager;

class ParallelPortManager : public DeviceBridge::IComponent {
private:
    Parallel::Port& _port;
    // Note: No longer storing direct references - using ServiceLocator
    
    // File detection state
    bool _fileInProgress;
    uint32_t _idleCounter;
    uint32_t _lastDataTime;
    
    // Data buffer for loop-based processing
    Common::DataChunk _currentChunk;
    uint16_t _chunkIndex;
    uint32_t _chunkStartTime;
    
    // File boundary detection
    bool detectNewFile();
    bool detectEndOfFile();
    
    // Data processing
    void processData();
    void sendChunk();
    bool shouldSendPartialChunk() const;
    
    // Critical timeout handling
    void handleCriticalTimeout();
    
public:
    ParallelPortManager(Parallel::Port& port);
    ~ParallelPortManager();
    
    // Lifecycle management (IComponent interface)
    bool initialize() override final;
    void update(unsigned long currentTime) override final;  // Called from main loop with timing
    void stop() override final;
    
    // IComponent interface implementation
    bool selfTest() override final;
    const char* getComponentName() const override final;
    bool validateDependencies() const override final;
    void printDependencyStatus() const override final;
    unsigned long getUpdateInterval() const override final;
    
    // Status inquiry
    bool isReceiving() const { return _fileInProgress; }
    uint16_t getBufferLevel() const;
    bool isBufferAlmostFull() const;
    bool isBufferCriticallyFull() const;
    
    // Statistics
    uint32_t getTotalBytesReceived() const;
    uint32_t getFilesReceived() const;
    
    // Debug methods
    uint32_t getInterruptCount() const;
    uint32_t getDataCount() const;
    
    // Control signal debugging
    bool isStrobeLow();
    bool isAutoFeedLow();
    bool isInitializeLow();
    bool isSelectInLow();
    
    // LPT locking for SPI/Serial operations
    void lockPort();
    void unlockPort();
    bool isPortLocked() const;
    
    // Printer protocol test methods
    void setPrinterBusy(bool busy);
    void setPrinterError(bool error);
    void setPrinterPaperOut(bool paperOut);
    void setPrinterSelect(bool select);
    void sendPrinterAcknowledge();
    
    // Buffer management methods
    void clearBuffer();
    uint16_t getBufferSize() const;
    
    // Critical state management
    bool isCriticalFlowControlActive() const;
    bool checkCriticalTimeout() const;
    void resetCriticalState();
    
    // Hardware flow control methods
    void setHardwareFlowControlEnabled(bool enabled);
    bool isHardwareFlowControlEnabled() const;
    DeviceBridge::Parallel::HardwareFlowControl::Statistics getFlowControlStatistics() const;
    
private:
    // Statistics tracking
    uint32_t _totalBytesReceived;
    uint32_t _filesReceived;
    uint32_t _currentFileBytes;
};

} // namespace DeviceBridge::Components