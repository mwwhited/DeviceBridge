#pragma once

#include <stdint.h>
#include <string.h>

namespace DeviceBridge::Common {

/**
 * Shared Buffer Pool for Memory Optimization
 * Consolidates scattered string buffers to reduce RAM usage
 * Thread-safe for single-threaded embedded environment
 */
class SharedBufferPool {
private:
    // Buffer pool - consolidated from individual component buffers
    static char small_buffer_1[32];    // General 32-byte operations
    static char small_buffer_2[32];    // Secondary 32-byte operations  
    static char medium_buffer[64];     // Larger operations (filenames, errors)
    static char large_buffer[80];      // Chunked data operations
    
    // Buffer usage tracking (bit flags for efficiency)
    static uint8_t buffer_usage_flags; // Bits: 0=small1, 1=small2, 2=medium, 3=large
    
public:
    enum BufferSize {
        SMALL = 32,   // For messages, component names, etc.
        MEDIUM = 64,  // For filenames, error messages  
        LARGE = 80    // For chunk data, complex operations
    };
    
    // Get buffer of specified size (returns nullptr if not available)
    static char* acquireBuffer(BufferSize size);
    
    // Release buffer back to pool
    static void releaseBuffer(char* buffer);
    
    // Check if buffer is available
    static bool isBufferAvailable(BufferSize size);
    
    // Emergency release all buffers (for error recovery)
    static void releaseAllBuffers();
    
    // Get buffer usage statistics
    static uint8_t getUsageFlags() { return buffer_usage_flags; }
    static uint8_t getAvailableCount();
    
private:
    // Buffer identification helpers
    static uint8_t getBufferIndex(char* buffer);
    static bool isValidBuffer(char* buffer);
};

} // namespace DeviceBridge::Common