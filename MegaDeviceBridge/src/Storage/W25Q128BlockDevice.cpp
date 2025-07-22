#include "W25Q128BlockDevice.h"
#include <string.h>

namespace DeviceBridge::Storage {

W25Q128BlockDevice::W25Q128BlockDevice(DeviceBridge::Components::W25Q128Manager* flash)
    : _flash(flash), _initialized(false) {
    memset(_cache_buffer, 0, sizeof(_cache_buffer));
    memset(_lookahead_buffer, 0, sizeof(_lookahead_buffer));
}

W25Q128BlockDevice::~W25Q128BlockDevice() {
    shutdown();
}

bool W25Q128BlockDevice::initialize() {
    if (!_flash || !_flash->isInitialized()) {
        return false;
    }
    
    _initialized = true;
    return true;
}

void W25Q128BlockDevice::shutdown() {
    _initialized = false;
}

int W25Q128BlockDevice::read(uint32_t block, uint32_t offset, void* buffer, uint32_t size) {
    if (!_initialized || !isValidBlock(block)) {
        return LFS_ERR_IO;
    }
    
    if (offset + size > BLOCK_SIZE) {
        return LFS_ERR_INVAL;
    }
    
    uint32_t address = blockToAddress(block) + offset;
    if (_flash->readData(address, (uint8_t*)buffer, size)) {
        return LFS_ERR_OK;
    }
    
    return LFS_ERR_IO;
}

int W25Q128BlockDevice::program(uint32_t block, uint32_t offset, const void* buffer, uint32_t size) {
    if (!_initialized || !isValidBlock(block)) {
        return LFS_ERR_IO;
    }
    
    if (offset + size > BLOCK_SIZE) {
        return LFS_ERR_INVAL;
    }
    
    uint32_t address = blockToAddress(block) + offset;
    
    // W25Q128 requires page-aligned writes for optimal performance
    // For non-aligned writes, we need to handle them carefully
    const uint8_t* data = (const uint8_t*)buffer;
    uint32_t remaining = size;
    uint32_t current_offset = offset;
    
    while (remaining > 0) {
        uint32_t page_offset = (address + current_offset) % _flash->getPageSize();
        uint32_t bytes_to_write = min(remaining, _flash->getPageSize() - page_offset);
        
        if (!_flash->writePage(address + current_offset, data + current_offset, bytes_to_write)) {
            return LFS_ERR_IO;
        }
        
        current_offset += bytes_to_write;
        remaining -= bytes_to_write;
    }
    
    return LFS_ERR_OK;
}

int W25Q128BlockDevice::erase(uint32_t block) {
    if (!_initialized || !isValidBlock(block)) {
        return LFS_ERR_IO;
    }
    
    uint32_t address = blockToAddress(block);
    if (_flash->eraseSector(address)) {
        return LFS_ERR_OK;
    }
    
    return LFS_ERR_IO;
}

int W25Q128BlockDevice::sync() {
    // W25Q128 writes are synchronous, no explicit sync needed
    return LFS_ERR_OK;
}

uint32_t W25Q128BlockDevice::getBadBlockCount() const {
    // W25Q128 is NOR flash, bad blocks are rare
    // For now, assume no bad blocks
    return 0;
}

uint32_t W25Q128BlockDevice::getEraseCount(uint32_t block) const {
    // Erase count tracking would require additional metadata
    // Not implemented in basic version
    return 0;
}

bool W25Q128BlockDevice::verifyBlock(uint32_t block) {
    if (!isValidBlock(block)) {
        return false;
    }
    
    // Read a test pattern to verify block integrity
    uint8_t test_buffer[16];
    uint32_t address = blockToAddress(block);
    
    return _flash->readData(address, test_buffer, sizeof(test_buffer));
}

} // namespace DeviceBridge::Storage

// C wrapper functions for LittleFS callbacks
extern "C" {

int w25q128_read(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size) {
    DeviceBridge::Storage::W25Q128BlockDevice* device = 
        (DeviceBridge::Storage::W25Q128BlockDevice*)c->context;
    return device->read(block, off, buffer, size);
}

int w25q128_prog(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size) {
    DeviceBridge::Storage::W25Q128BlockDevice* device = 
        (DeviceBridge::Storage::W25Q128BlockDevice*)c->context;
    return device->program(block, off, buffer, size);
}

int w25q128_erase(const struct lfs_config *c, lfs_block_t block) {
    DeviceBridge::Storage::W25Q128BlockDevice* device = 
        (DeviceBridge::Storage::W25Q128BlockDevice*)c->context;
    return device->erase(block);
}

int w25q128_sync(const struct lfs_config *c) {
    DeviceBridge::Storage::W25Q128BlockDevice* device = 
        (DeviceBridge::Storage::W25Q128BlockDevice*)c->context;
    return device->sync();
}

} // extern "C"