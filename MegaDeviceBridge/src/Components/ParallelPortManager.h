#pragma once

#include <Arduino.h>
#include "../Parallel/Port.h"
#include "../Common/Types.h"
#include "../Common/Config.h"

namespace DeviceBridge::Components {

// Forward declaration for callback
class FileSystemManager;

class ParallelPortManager {
private:
    Parallel::Port& _port;
    FileSystemManager* _fileSystemManager;
    
    // File detection state
    bool _fileInProgress;
    uint32_t _idleCounter;
    uint32_t _lastDataTime;
    
    // Data buffer for loop-based processing
    Common::DataChunk _currentChunk;
    uint16_t _chunkIndex;
    
    // File boundary detection
    bool detectNewFile();
    bool detectEndOfFile();
    
    // Data processing
    void processData();
    void sendChunk();
    
public:
    ParallelPortManager(Parallel::Port& port);
    ~ParallelPortManager();
    
    // Set callback for data delivery
    void setFileSystemManager(FileSystemManager* manager) { _fileSystemManager = manager; }
    
    // Lifecycle management
    bool initialize();
    void update();  // Called from main loop
    void stop();
    
    // Status inquiry
    bool isReceiving() const { return _fileInProgress; }
    uint16_t getBufferLevel() const;
    
    // Statistics
    uint32_t getTotalBytesReceived() const;
    uint32_t getFilesReceived() const;
    
    // Debug methods
    uint32_t getInterruptCount() const;
    uint32_t getDataCount() const;
    
private:
    // Statistics tracking
    uint32_t _totalBytesReceived;
    uint32_t _filesReceived;
    uint32_t _currentFileBytes;
};

} // namespace DeviceBridge::Components