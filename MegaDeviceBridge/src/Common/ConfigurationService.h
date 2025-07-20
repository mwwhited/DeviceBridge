#pragma once

#include <Arduino.h>
#include "Config.h"

namespace DeviceBridge::Common {

/**
 * Configuration service providing centralized access to all system configuration values
 * Eliminates magic numbers throughout the codebase by providing typed access to constants
 */
class ConfigurationService {
public:
    // Timing configuration access
    static constexpr unsigned long getParallelInterval() { return Timing::PARALLEL_INTERVAL; }
    static constexpr unsigned long getFileSystemInterval() { return Timing::FILESYSTEM_INTERVAL; }
    static constexpr unsigned long getDisplayInterval() { return Timing::DISPLAY_INTERVAL; }
    static constexpr unsigned long getTimeInterval() { return Timing::TIME_INTERVAL; }
    static constexpr unsigned long getSystemInterval() { return Timing::SYSTEM_INTERVAL; }
    static constexpr unsigned long getHeartbeatInterval() { return Timing::HEARTBEAT_INTERVAL; }
    static constexpr unsigned long getConfigurationInterval() { return Timing::CONFIGURATION_INTERVAL; }
    
    // Microsecond timing access
    static constexpr uint16_t getAckPulseUs() { return Timing::ACK_PULSE_US; }
    static constexpr uint16_t getRecoveryDelayUs() { return Timing::RECOVERY_DELAY_US; }
    static constexpr uint16_t getHardwareDelayUs() { return Timing::HARDWARE_DELAY_US; }
    static constexpr uint16_t getTds2024TimingUs() { return Timing::TDS2024_TIMING_US; }
    static constexpr uint16_t getFlowControlDelayUs() { return Timing::FLOW_CONTROL_DELAY_US; }
    static constexpr uint16_t getModerateFlowDelayUs() { return Timing::MODERATE_FLOW_DELAY_US; }
    static constexpr uint16_t getCriticalFlowDelayUs() { return Timing::CRITICAL_FLOW_DELAY_US; }
    
    // Millisecond delays access
    static constexpr uint16_t getEmergencyRecoveryMs() { return Timing::EMERGENCY_RECOVERY_MS; }
    static constexpr uint16_t getGeneralDelayMs() { return Timing::GENERAL_DELAY_MS; }
    static constexpr uint16_t getKeepBusyMs() { return Timing::KEEP_BUSY_MS; }
    static constexpr uint16_t getShortDelayMs() { return Timing::SHORT_DELAY_MS; }
    
    // Buffer configuration access
    static constexpr uint16_t getRingBufferSize() { return Buffer::RING_BUFFER_SIZE; }
    static constexpr uint16_t getEepromBufferSize() { return Buffer::EEPROM_BUFFER_SIZE; }
    static constexpr uint32_t getCriticalTimeoutMs() { return Buffer::CRITICAL_TIMEOUT_MS; }
    
    // Flow control threshold calculation - OPTIMIZED FOR TDS2024
    static constexpr uint16_t getPreWarningFlowThreshold(uint16_t bufferSize) {
        return (bufferSize * Buffer::FLOW_CONTROL_40_PERCENT) / Buffer::FLOW_CONTROL_40_DIVISOR;
    }
    
    static constexpr uint16_t getModerateFlowThreshold(uint16_t bufferSize) {
        return (bufferSize * Buffer::FLOW_CONTROL_50_PERCENT) / Buffer::FLOW_CONTROL_50_DIVISOR;
    }
    
    static constexpr uint16_t getCriticalFlowThreshold(uint16_t bufferSize) {
        return (bufferSize * Buffer::FLOW_CONTROL_70_PERCENT) / Buffer::FLOW_CONTROL_70_DIVISOR;
    }
    
    static constexpr uint16_t getRecoveryFlowThreshold(uint16_t bufferSize) {
        return (bufferSize * Buffer::FLOW_CONTROL_40_PERCENT) / Buffer::FLOW_CONTROL_40_DIVISOR;
    }
    
    // Legacy flow control thresholds for compatibility
    static constexpr uint16_t getLegacyModerateFlowThreshold(uint16_t bufferSize) {
        return (bufferSize * Buffer::FLOW_CONTROL_60_PERCENT) / Buffer::FLOW_CONTROL_60_DIVISOR;
    }
    
    static constexpr uint16_t getLegacyCriticalFlowThreshold(uint16_t bufferSize) {
        return (bufferSize * Buffer::FLOW_CONTROL_80_PERCENT) / Buffer::FLOW_CONTROL_80_DIVISOR;
    }
    
    // Button configuration access
    static constexpr uint16_t getButtonRightValue() { return Buttons::BUTTON_RIGHT_VALUE; }
    static constexpr uint16_t getButtonUpValue() { return Buttons::BUTTON_UP_VALUE; }
    static constexpr uint16_t getButtonDownValue() { return Buttons::BUTTON_DOWN_VALUE; }
    static constexpr uint16_t getButtonLeftValue() { return Buttons::BUTTON_LEFT_VALUE; }
    static constexpr uint16_t getButtonSelectValue() { return Buttons::BUTTON_SELECT_VALUE; }
    static constexpr uint16_t getButtonNoneValue() { return Buttons::BUTTON_NONE_VALUE; }
    
    static constexpr uint16_t getRightThreshold() { return Buttons::RIGHT_THRESHOLD; }
    static constexpr uint16_t getUpThreshold() { return Buttons::UP_THRESHOLD; }
    static constexpr uint16_t getDownThreshold() { return Buttons::DOWN_THRESHOLD; }
    static constexpr uint16_t getLeftThreshold() { return Buttons::LEFT_THRESHOLD; }
    static constexpr uint16_t getSelectThreshold() { return Buttons::SELECT_THRESHOLD; }
    
    // File format detection access
    static constexpr uint8_t getBmpSignature1() { return FileFormats::BMP_SIGNATURE_1; }
    static constexpr uint8_t getBmpSignature2() { return FileFormats::BMP_SIGNATURE_2; }
    static constexpr uint8_t getPcxSignature() { return FileFormats::PCX_SIGNATURE; }
    static constexpr uint8_t getEscCharacter() { return FileFormats::ESC_CHARACTER; }
    static constexpr uint8_t getPsSignature1() { return FileFormats::PS_SIGNATURE_1; }
    static constexpr uint8_t getPsSignature2() { return FileFormats::PS_SIGNATURE_2; }
    
    // TIFF signatures
    static constexpr bool isTiffLittleEndian(uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4) {
        return (b1 == FileFormats::TIFF_LE_1 && b2 == FileFormats::TIFF_LE_2 && 
                b3 == FileFormats::TIFF_LE_3 && b4 == FileFormats::TIFF_LE_4);
    }
    
    static constexpr bool isTiffBigEndian(uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4) {
        return (b1 == FileFormats::TIFF_BE_1 && b2 == FileFormats::TIFF_BE_2 && 
                b3 == FileFormats::TIFF_BE_3 && b4 == FileFormats::TIFF_BE_4);
    }
    
    // Flash memory configuration access
    static constexpr uint32_t getFlashPageSize() { return Flash::PAGE_SIZE; }
    static constexpr uint32_t getFlashSectorSize() { return Flash::SECTOR_SIZE; }
    static constexpr uint32_t getW25Q128JedecId() { return Flash::W25Q128_JEDEC_ID; }
    
    // Flash command access
    static constexpr uint8_t getWriteEnableCmd() { return Flash::CMD_WRITE_ENABLE; }
    static constexpr uint8_t getWriteDisableCmd() { return Flash::CMD_WRITE_DISABLE; }
    static constexpr uint8_t getReadStatusCmd() { return Flash::CMD_READ_STATUS; }
    static constexpr uint8_t getWriteStatusCmd() { return Flash::CMD_WRITE_STATUS; }
    static constexpr uint8_t getPageProgramCmd() { return Flash::CMD_PAGE_PROGRAM; }
    static constexpr uint8_t getSectorEraseCmd() { return Flash::CMD_SECTOR_ERASE; }
    static constexpr uint8_t getBlockErase32KCmd() { return Flash::CMD_BLOCK_ERASE_32K; }
    static constexpr uint8_t getBlockErase64KCmd() { return Flash::CMD_BLOCK_ERASE_64K; }
    static constexpr uint8_t getChipEraseCmd() { return Flash::CMD_CHIP_ERASE; }
    static constexpr uint8_t getReadDataCmd() { return Flash::CMD_READ_DATA; }
    static constexpr uint8_t getFastReadCmd() { return Flash::CMD_FAST_READ; }
    static constexpr uint8_t getReadJedecIdCmd() { return Flash::CMD_READ_JEDEC_ID; }
    static constexpr uint8_t getPowerDownCmd() { return Flash::CMD_POWER_DOWN; }
    static constexpr uint8_t getReleasePowerDownCmd() { return Flash::CMD_RELEASE_POWER_DOWN; }
    
    // Display refresh configuration access
    static constexpr uint32_t getNormalDisplayInterval() { return DisplayRefresh::NORMAL_INTERVAL_MS; }
    static constexpr uint32_t getStorageDisplayInterval() { return DisplayRefresh::STORAGE_INTERVAL_MS; }
    static constexpr uint8_t getLcdWidth() { return DisplayRefresh::LCD_WIDTH; }
    static constexpr uint8_t getLcdHeight() { return DisplayRefresh::LCD_HEIGHT; }
    
    // Flow control percentage access - OPTIMIZED FOR TDS2024
    static constexpr uint8_t getPreWarningThresholdPercent() { return FlowControl::PRE_WARNING_THRESHOLD_PERCENT; }
    static constexpr uint8_t getModerateThresholdPercent() { return FlowControl::MODERATE_THRESHOLD_PERCENT; }
    static constexpr uint8_t getCriticalThresholdPercent() { return FlowControl::CRITICAL_THRESHOLD_PERCENT; }
    static constexpr uint8_t getRecoveryThresholdPercent() { return FlowControl::RECOVERY_THRESHOLD_PERCENT; }
    
    // Pin configuration access (delegating to existing Pins namespace)
    static constexpr uint8_t getHeartbeatPin() { return Pins::HEARTBEAT; }
    static constexpr uint8_t getLcdResetPin() { return Pins::LCD_RESET; }
    static constexpr uint8_t getLcdEnablePin() { return Pins::LCD_ENABLE; }
    static constexpr uint8_t getLcdD4Pin() { return Pins::LCD_D4; }
    static constexpr uint8_t getLcdD5Pin() { return Pins::LCD_D5; }
    static constexpr uint8_t getLcdD6Pin() { return Pins::LCD_D6; }
    static constexpr uint8_t getLcdD7Pin() { return Pins::LCD_D7; }
    static constexpr uint8_t getLcdButtonsPin() { return Pins::LCD_BUTTONS; }
    static constexpr uint8_t getSdCsPin() { return Pins::SD_CS; }
    static constexpr uint8_t getEepromCsPin() { return Pins::EEPROM_CS; }
    static constexpr uint8_t getSdCdPin() { return Pins::SD_CD; }
    static constexpr uint8_t getSdWpPin() { return Pins::SD_WP; }
    static constexpr uint8_t getLptReadLedPin() { return Pins::LPT_READ_LED; }
    static constexpr uint8_t getDataWriteLedPin() { return Pins::DATA_WRITE_LED; }
    static constexpr uint8_t getLptStrobePin() { return Pins::LPT_STROBE; }
    static constexpr uint8_t getLptAutoFeedPin() { return Pins::LPT_AUTO_FEED; }
    static constexpr uint8_t getLptInitializePin() { return Pins::LPT_INITIALIZE; }
    static constexpr uint8_t getLptSelectInPin() { return Pins::LPT_SELECT_IN; }
    static constexpr uint8_t getLptAckPin() { return Pins::LPT_ACK; }
    static constexpr uint8_t getLptBusyPin() { return Pins::LPT_BUSY; }
    static constexpr uint8_t getLptPaperOutPin() { return Pins::LPT_PAPER_OUT; }
    static constexpr uint8_t getLptSelectPin() { return Pins::LPT_SELECT; }
    static constexpr uint8_t getLptErrorPin() { return Pins::LPT_ERROR; }
    static constexpr uint8_t getLptD0Pin() { return Pins::LPT_D0; }
    static constexpr uint8_t getLptD1Pin() { return Pins::LPT_D1; }
    static constexpr uint8_t getLptD2Pin() { return Pins::LPT_D2; }
    static constexpr uint8_t getLptD3Pin() { return Pins::LPT_D3; }
    static constexpr uint8_t getLptD4Pin() { return Pins::LPT_D4; }
    static constexpr uint8_t getLptD5Pin() { return Pins::LPT_D5; }
    static constexpr uint8_t getLptD6Pin() { return Pins::LPT_D6; }
    static constexpr uint8_t getLptD7Pin() { return Pins::LPT_D7; }
    
    // Debug configuration access
    static constexpr uint8_t getHeaderHexBytes() { return Debug::HEADER_HEX_BYTES; }
};

} // namespace DeviceBridge::Common