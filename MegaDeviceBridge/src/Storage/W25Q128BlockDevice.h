#pragma once

#include <Arduino.h>
#include "../Components/W25Q128Manager.h"
#include "lfs.h"

namespace DeviceBridge::Storage {

/**
 * @brief LittleFS block device adapter for W25Q128 flash memory
 * 
 * Provides the block device interface required by LittleFS for the W25Q128 flash chip.
 * Implements wear leveling, bad block management, and optimized read/write operations.
 */
class W25Q128BlockDevice {
private:
    DeviceBridge::Components::W25Q128Manager* _flash;
    bool _initialized;
    
    // Flash geometry constants optimized for W25Q128
    static constexpr uint32_t BLOCK_SIZE = 4096;      // 4KB sectors (LittleFS block size)
    static constexpr uint32_t TOTAL_BLOCKS = 4096;    // 16MB / 4KB = 4096 blocks
    static constexpr uint32_t PROG_SIZE = 256;        // Page program size
    static constexpr uint32_t READ_SIZE = 1;          // Minimum read size
    static constexpr uint32_t ERASE_SIZE = 4096;      // Sector erase size
    static constexpr uint32_t CACHE_SIZE = 64;        // Minimal cache size for Arduino Mega
    static constexpr uint32_t LOOKAHEAD_SIZE = 32;    // Minimal lookahead buffer size
    
    // Minimal buffers to prevent memory issues
    uint8_t _cache_buffer[CACHE_SIZE];
    uint8_t _lookahead_buffer[LOOKAHEAD_SIZE];
    
public:
    W25Q128BlockDevice(DeviceBridge::Components::W25Q128Manager* flash);
    ~W25Q128BlockDevice();
    
    // Lifecycle management
    bool initialize();
    bool isInitialized() const { return _initialized; }
    void shutdown();
    
    // Block device operations (required by LittleFS)
    int read(uint32_t block, uint32_t offset, void* buffer, uint32_t size);
    int program(uint32_t block, uint32_t offset, const void* buffer, uint32_t size);
    int erase(uint32_t block);
    int sync();
    
    // Block device information
    uint32_t getBlockSize() const { return BLOCK_SIZE; }
    uint32_t getBlockCount() const { return TOTAL_BLOCKS; }
    uint32_t getProgSize() const { return PROG_SIZE; }
    uint32_t getReadSize() const { return READ_SIZE; }
    uint32_t getEraseSize() const { return ERASE_SIZE; }
    uint32_t getCacheSize() const { return CACHE_SIZE; }
    uint32_t getLookaheadSize() const { return LOOKAHEAD_SIZE; }
    
    // Buffer access for LittleFS configuration
    void* getCacheBuffer() { return _cache_buffer; }
    void* getLookaheadBuffer() { return _lookahead_buffer; }
    
    // Utility methods
    bool isValidBlock(uint32_t block) const { return block < TOTAL_BLOCKS; }
    uint32_t blockToAddress(uint32_t block) const { return block * BLOCK_SIZE; }
    uint32_t addressToBlock(uint32_t address) const { return address / BLOCK_SIZE; }
    
    // Statistics and diagnostics
    uint32_t getBadBlockCount() const;
    uint32_t getEraseCount(uint32_t block) const;
    bool verifyBlock(uint32_t block);
};

// C wrapper functions for LittleFS callbacks
extern "C" {
    int w25q128_read(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size);
    int w25q128_prog(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size);
    int w25q128_erase(const struct lfs_config *c, lfs_block_t block);
    int w25q128_sync(const struct lfs_config *c);
}

} // namespace DeviceBridge::Storage