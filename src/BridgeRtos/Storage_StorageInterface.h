#pragma once

#include <stdint.h>

namespace DeviceBridge::Storage
{
    class StorageInterface
    {
    public:
        virtual void initialize() {};

        // TODO: Create/append file
        // TODO: Select File
        // TODO: Read/Stream file
        // TODO: Delete file
    };
}