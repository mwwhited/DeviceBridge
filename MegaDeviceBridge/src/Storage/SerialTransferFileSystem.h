#pragma once

#include "IFileSystem.h"
#include "../Common/Config.h"
#include <Arduino.h>

namespace DeviceBridge::Storage {

/**
 * @brief Serial Transfer file system implementation
 * 
 * Provides file operations for serial transfer storage where data is
 * transmitted in real-time over the serial interface with protocol headers,
 * progress indication, and transfer verification.
 */
class SerialTransferFileSystem : public IFileSystem {
private:
    bool _initialized;
    bool _transferInProgress;
    char _currentFilename[64];
    uint32_t _currentFileSize;
    uint32_t _transferredBytes;
    
    // Serial transfer protocol constants
    static constexpr uint8_t PROTOCOL_VERSION = 1;
    static constexpr uint8_t START_MARKER = 0xAA;
    static constexpr uint8_t END_MARKER = 0x55;
    static constexpr uint16_t MAX_CHUNK_SIZE = 64;    // Maximum data chunk per packet
    static constexpr uint16_t HEADER_SIZE = 16;       // Protocol header size
    
    // Transfer protocol packets
    struct TransferHeader {
        uint8_t startMarker;      // START_MARKER (0xAA)
        uint8_t version;          // PROTOCOL_VERSION
        uint8_t packetType;       // Packet type (header, data, end)
        uint8_t reserved;         // Reserved for future use
        uint32_t fileSize;        // Total file size
        uint32_t sequenceNumber;  // Packet sequence number
        uint16_t dataLength;      // Length of data in this packet
        uint16_t checksum;        // Packet checksum
    };
    
    enum PacketType {
        PACKET_FILE_HEADER = 1,   // File start packet with metadata
        PACKET_DATA = 2,          // Data chunk packet
        PACKET_FILE_END = 3       // File end packet with verification
    };
    
    // Private methods
    bool sendTransferHeader(const char* filename, uint32_t fileSize);
    bool sendDataChunk(const uint8_t* data, uint16_t length);
    bool sendTransferEnd();
    uint16_t calculatePacketChecksum(const TransferHeader& header, const uint8_t* data);
    void sendProgressUpdate();
    bool waitForAcknowledgment(uint32_t timeoutMs = 1000);
    
public:
    SerialTransferFileSystem();
    virtual ~SerialTransferFileSystem();
    
    // Lifecycle management
    bool initialize() override final;
    bool isAvailable() const override final;
    void shutdown() override final;
    
    // File operations
    bool createFile(const char* filename) override final;
    bool openFile(const char* filename, bool append = false) override final;
    bool writeData(const uint8_t* data, uint16_t length) override final;
    bool closeFile() override final;
    bool deleteFile(const char* filename) override final;
    bool fileExists(const char* filename) override final;
    
    // Directory operations
    bool listFiles(char* buffer, uint16_t bufferSize) override final;
    uint32_t getFileCount() override final;
    uint32_t getTotalSpace() override final;
    uint32_t getFreeSpace() override final;
    
    // Status inquiry
    Common::StorageType getStorageType() const override { 
        return Common::StorageType(Common::StorageType::SERIAL_TRANSFER); 
    }
    const char* getStorageName() const override { return "Serial Transfer"; }
    bool isWriteProtected() const override { return false; }
    bool hasActiveFile() const override { return _hasActiveFile; }
    
    // Statistics
    uint32_t getBytesWritten() const override { return _bytesWritten; }
    uint32_t getFilesCreated() const override { return _filesCreated; }
    uint16_t getLastError() const override { return _lastError; }
    const char* getLastErrorMessage() const override { return _lastErrorMessage; }
    
    // Serial transfer doesn't support these operations
    bool format() override { return false; }
    bool flush() override { return true; }  // Data sent immediately
    bool sync() override { return true; }   // Data sent immediately
    
    // Serial Transfer specific methods
    bool isTransferInProgress() const { return _transferInProgress; }
    uint32_t getTransferProgress() const; // Returns percentage (0-100)
    void setProgressCallback(void (*callback)(uint32_t percent));
    bool enableBinaryMode(bool enable);   // Switch between text and binary transfer
    bool setTransferSpeed(uint32_t baudRate); // Change serial speed for transfer
    
private:
    void (*_progressCallback)(uint32_t percent) = nullptr;
    bool _binaryMode = true;              // Default to binary transfer
    uint32_t _transferBaudRate = 115200;  // Default transfer speed
};

} // namespace DeviceBridge::Storage