#pragma once

#include <stdint.h>
#include <Arduino_FreeRTOS.h>

namespace DeviceBridge::Tasks
{
    class TaskInterface
    {
    public:
        virtual void initialize() {};

    private:
        TaskHandle_t Handler;
    };
}