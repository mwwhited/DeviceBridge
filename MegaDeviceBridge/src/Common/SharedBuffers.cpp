#include "SharedBuffers.h"
#include <Arduino.h>

namespace DeviceBridge::Common {

// Static buffer definitions
char SharedBufferPool::small_buffer_1[32];
char SharedBufferPool::small_buffer_2[32]; 
char SharedBufferPool::medium_buffer[64];
char SharedBufferPool::large_buffer[80];
uint8_t SharedBufferPool::buffer_usage_flags = 0;

char* SharedBufferPool::acquireBuffer(BufferSize size) {
    switch (size) {
        case SMALL:
            // Try small_buffer_1 first
            if (!(buffer_usage_flags & 0x01)) {
                buffer_usage_flags |= 0x01;
                memset(small_buffer_1, 0, sizeof(small_buffer_1));
                return small_buffer_1;
            }
            // Try small_buffer_2
            if (!(buffer_usage_flags & 0x02)) {
                buffer_usage_flags |= 0x02;
                memset(small_buffer_2, 0, sizeof(small_buffer_2));
                return small_buffer_2;
            }
            break;
            
        case MEDIUM:
            if (!(buffer_usage_flags & 0x04)) {
                buffer_usage_flags |= 0x04;
                memset(medium_buffer, 0, sizeof(medium_buffer));
                return medium_buffer;
            }
            break;
            
        case LARGE:
            if (!(buffer_usage_flags & 0x08)) {
                buffer_usage_flags |= 0x08;
                memset(large_buffer, 0, sizeof(large_buffer));
                return large_buffer;
            }
            break;
    }
    
    return nullptr; // No buffer available
}

void SharedBufferPool::releaseBuffer(char* buffer) {
    if (!isValidBuffer(buffer)) {
        return; // Invalid buffer pointer
    }
    
    uint8_t index = getBufferIndex(buffer);
    if (index < 4) {
        buffer_usage_flags &= ~(1 << index); // Clear the corresponding bit
    }
}

bool SharedBufferPool::isBufferAvailable(BufferSize size) {
    switch (size) {
        case SMALL:
            return !(buffer_usage_flags & 0x01) || !(buffer_usage_flags & 0x02);
        case MEDIUM:
            return !(buffer_usage_flags & 0x04);
        case LARGE:
            return !(buffer_usage_flags & 0x08);
    }
    return false;
}

void SharedBufferPool::releaseAllBuffers() {
    buffer_usage_flags = 0;
    // Optional: Clear all buffers for security
    memset(small_buffer_1, 0, sizeof(small_buffer_1));
    memset(small_buffer_2, 0, sizeof(small_buffer_2));
    memset(medium_buffer, 0, sizeof(medium_buffer));
    memset(large_buffer, 0, sizeof(large_buffer));
}

uint8_t SharedBufferPool::getAvailableCount() {
    uint8_t available = 0;
    for (uint8_t i = 0; i < 4; i++) {
        if (!(buffer_usage_flags & (1 << i))) {
            available++;
        }
    }
    return available;
}

uint8_t SharedBufferPool::getBufferIndex(char* buffer) {
    if (buffer == small_buffer_1) return 0;
    if (buffer == small_buffer_2) return 1;
    if (buffer == medium_buffer) return 2;
    if (buffer == large_buffer) return 3;
    return 0xFF; // Invalid
}

bool SharedBufferPool::isValidBuffer(char* buffer) {
    return buffer == small_buffer_1 || 
           buffer == small_buffer_2 || 
           buffer == medium_buffer || 
           buffer == large_buffer;
}

} // namespace DeviceBridge::Common