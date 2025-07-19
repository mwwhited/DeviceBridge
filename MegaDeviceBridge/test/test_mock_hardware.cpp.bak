#include <unity.h>
#include <Arduino.h>

// Mock hardware simulation for testing component integration
class MockParallelPort {
private:
    uint8_t _mockData[1024];
    uint16_t _readIndex;
    uint16_t _writeIndex;
    bool _hasData;
    
public:
    MockParallelPort() : _readIndex(0), _writeIndex(0), _hasData(false) {
        memset(_mockData, 0, sizeof(_mockData));
    }
    
    void simulateDataReceived(const uint8_t* data, uint16_t length) {
        for (uint16_t i = 0; i < length && _writeIndex < sizeof(_mockData); i++) {
            _mockData[_writeIndex++] = data[i];
        }
        _hasData = (_writeIndex > _readIndex);
    }
    
    bool hasData() const {
        return _hasData && (_readIndex < _writeIndex);
    }
    
    uint16_t readData(uint8_t* buffer, uint16_t length) {
        uint16_t bytesRead = 0;
        
        while (bytesRead < length && _readIndex < _writeIndex) {
            buffer[bytesRead++] = _mockData[_readIndex++];
        }
        
        _hasData = (_readIndex < _writeIndex);
        return bytesRead;
    }
    
    void reset() {
        _readIndex = 0;
        _writeIndex = 0;
        _hasData = false;
        memset(_mockData, 0, sizeof(_mockData));
    }
};

class MockDisplay {
private:
    char _currentMessage[32];
    char _currentLine2[32];
    bool _initialized;
    
public:
    MockDisplay() : _initialized(false) {
        memset(_currentMessage, 0, sizeof(_currentMessage));
        memset(_currentLine2, 0, sizeof(_currentLine2));
    }
    
    void initialize() {
        _initialized = true;
        strcpy(_currentMessage, "Device Bridge");
        strcpy(_currentLine2, "Initializing...");
    }
    
    void updateStatus(const char* message) {
        if (_initialized && message) {
            strncpy(_currentMessage, message, sizeof(_currentMessage) - 1);
            _currentMessage[sizeof(_currentMessage) - 1] = '\0';
            _currentLine2[0] = '\0';
        }
    }
    
    void showTime(const char* timeStr) {
        if (_initialized && timeStr) {
            strcpy(_currentMessage, "Device Bridge");
            strncpy(_currentLine2, timeStr, sizeof(_currentLine2) - 1);
            _currentLine2[sizeof(_currentLine2) - 1] = '\0';
        }
    }
    
    void showMenu(const char* line1, const char* line2) {
        if (_initialized && line1) {
            strncpy(_currentMessage, line1, sizeof(_currentMessage) - 1);
            _currentMessage[sizeof(_currentMessage) - 1] = '\0';
            
            if (line2) {
                strncpy(_currentLine2, line2, sizeof(_currentLine2) - 1);
                _currentLine2[sizeof(_currentLine2) - 1] = '\0';
            } else {
                _currentLine2[0] = '\0';
            }
        }
    }
    
    const char* getCurrentMessage() const { return _currentMessage; }
    const char* getCurrentLine2() const { return _currentLine2; }
    bool isInitialized() const { return _initialized; }
};

class MockSDCard {
private:
    struct MockFile {
        char name[32];
        uint8_t data[1024];
        uint16_t size;
        bool isOpen;
    };
    
    MockFile _files[8]; // Support up to 8 mock files
    uint8_t _fileCount;
    int8_t _currentFile;
    bool _available;
    
public:
    MockSDCard() : _fileCount(0), _currentFile(-1), _available(true) {
        memset(_files, 0, sizeof(_files));
    }
    
    bool begin(uint8_t csPin) {
        (void)csPin; // Unused in mock
        return _available;
    }
    
    bool open(const char* filename, uint8_t mode) {
        if (!_available || !filename || _fileCount >= 8) {
            return false;
        }
        
        // Find existing file or create new one
        for (uint8_t i = 0; i < _fileCount; i++) {
            if (strcmp(_files[i].name, filename) == 0) {
                _currentFile = i;
                _files[i].isOpen = true;
                if (mode & 0x02) { // Write mode - truncate
                    _files[i].size = 0;
                    memset(_files[i].data, 0, sizeof(_files[i].data));
                }
                return true;
            }
        }
        
        // Create new file
        if (_fileCount < 8) {
            strncpy(_files[_fileCount].name, filename, sizeof(_files[_fileCount].name) - 1);
            _files[_fileCount].name[sizeof(_files[_fileCount].name) - 1] = '\0';
            _files[_fileCount].size = 0;
            _files[_fileCount].isOpen = true;
            memset(_files[_fileCount].data, 0, sizeof(_files[_fileCount].data));
            _currentFile = _fileCount++;
            return true;
        }
        
        return false;
    }
    
    size_t write(const uint8_t* buffer, size_t length) {
        if (_currentFile < 0 || !_files[_currentFile].isOpen || !buffer) {
            return 0;
        }
        
        MockFile& file = _files[_currentFile];
        size_t bytesToWrite = min(length, sizeof(file.data) - file.size);
        
        memcpy(file.data + file.size, buffer, bytesToWrite);
        file.size += bytesToWrite;
        
        return bytesToWrite;
    }
    
    void close() {
        if (_currentFile >= 0) {
            _files[_currentFile].isOpen = false;
            _currentFile = -1;
        }
    }
    
    void sync() {
        // Mock - nothing to do
    }
    
    bool isOpen() const {
        return _currentFile >= 0 && _files[_currentFile].isOpen;
    }
    
    // Test helpers
    uint8_t getFileCount() const { return _fileCount; }
    const char* getFileName(uint8_t index) const {
        return (index < _fileCount) ? _files[index].name : nullptr;
    }
    uint16_t getFileSize(uint8_t index) const {
        return (index < _fileCount) ? _files[index].size : 0;
    }
    void setAvailable(bool available) { _available = available; }
};

// Test instances
MockParallelPort mockPort;
MockDisplay mockDisplay;
MockSDCard mockSD;

// Test function prototypes
void test_mock_parallel_port();
void test_mock_display();
void test_mock_sd_card();
void test_data_flow_simulation();

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    UNITY_BEGIN();
    
    RUN_TEST(test_mock_parallel_port);
    RUN_TEST(test_mock_display);
    RUN_TEST(test_mock_sd_card);
    RUN_TEST(test_data_flow_simulation);
    
    UNITY_END();
}

void loop() {
    // Empty
}

void test_mock_parallel_port() {
    mockPort.reset();
    
    // Test initial state
    TEST_ASSERT_FALSE(mockPort.hasData());
    
    // Simulate data arrival
    uint8_t testData[] = {0x01, 0x02, 0x03, 0x04};
    mockPort.simulateDataReceived(testData, sizeof(testData));
    
    TEST_ASSERT_TRUE(mockPort.hasData());
    
    // Test data reading
    uint8_t buffer[10];
    uint16_t bytesRead = mockPort.readData(buffer, sizeof(buffer));
    
    TEST_ASSERT_EQUAL(4, bytesRead);
    TEST_ASSERT_EQUAL(0x01, buffer[0]);
    TEST_ASSERT_EQUAL(0x02, buffer[1]);
    TEST_ASSERT_EQUAL(0x03, buffer[2]);
    TEST_ASSERT_EQUAL(0x04, buffer[3]);
    
    // Should have no more data
    TEST_ASSERT_FALSE(mockPort.hasData());
}

void test_mock_display() {
    // Test initialization
    TEST_ASSERT_FALSE(mockDisplay.isInitialized());
    
    mockDisplay.initialize();
    TEST_ASSERT_TRUE(mockDisplay.isInitialized());
    TEST_ASSERT_EQUAL_STRING("Device Bridge", mockDisplay.getCurrentMessage());
    
    // Test status update
    mockDisplay.updateStatus("Ready");
    TEST_ASSERT_EQUAL_STRING("Ready", mockDisplay.getCurrentMessage());
    
    // Test time display
    mockDisplay.showTime("12:34:56");
    TEST_ASSERT_EQUAL_STRING("Device Bridge", mockDisplay.getCurrentMessage());
    TEST_ASSERT_EQUAL_STRING("12:34:56", mockDisplay.getCurrentLine2());
    
    // Test menu display
    mockDisplay.showMenu("Main Menu", ">Storage");
    TEST_ASSERT_EQUAL_STRING("Main Menu", mockDisplay.getCurrentMessage());
    TEST_ASSERT_EQUAL_STRING(">Storage", mockDisplay.getCurrentLine2());
}

void test_mock_sd_card() {
    // Test initialization
    TEST_ASSERT_TRUE(mockSD.begin(10));
    
    // Test file creation
    TEST_ASSERT_TRUE(mockSD.open("test.bin", 0x02)); // Write mode
    TEST_ASSERT_TRUE(mockSD.isOpen());
    
    // Test writing
    uint8_t testData[] = {0xDE, 0xAD, 0xBE, 0xEF};
    size_t written = mockSD.write(testData, sizeof(testData));
    TEST_ASSERT_EQUAL(4, written);
    
    mockSD.sync();
    mockSD.close();
    TEST_ASSERT_FALSE(mockSD.isOpen());
    
    // Verify file was created
    TEST_ASSERT_EQUAL(1, mockSD.getFileCount());
    TEST_ASSERT_EQUAL_STRING("test.bin", mockSD.getFileName(0));
    TEST_ASSERT_EQUAL(4, mockSD.getFileSize(0));
    
    // Test SD card failure simulation
    mockSD.setAvailable(false);
    TEST_ASSERT_FALSE(mockSD.begin(10));
    
    mockSD.setAvailable(true);
    TEST_ASSERT_TRUE(mockSD.begin(10));
}

void test_data_flow_simulation() {
    // Simulate complete data capture and storage workflow
    mockPort.reset();
    mockDisplay.initialize();
    mockSD.begin(10);
    
    // Simulate TDS2024 sending data
    uint8_t waveformData[] = {
        0x42, 0x4D,             // BMP header "BM"
        0x36, 0x84, 0x03, 0x00, // File size
        0x00, 0x00, 0x00, 0x00, // Reserved
        0x36, 0x00, 0x00, 0x00, // Data offset
        // ... more BMP data would follow
    };
    
    // Stage 1: Data arrives at parallel port
    mockPort.simulateDataReceived(waveformData, sizeof(waveformData));
    TEST_ASSERT_TRUE(mockPort.hasData());
    
    // Stage 2: Display shows receiving status
    mockDisplay.updateStatus("Receiving...");
    TEST_ASSERT_EQUAL_STRING("Receiving...", mockDisplay.getCurrentMessage());
    
    // Stage 3: Create file for storage
    TEST_ASSERT_TRUE(mockSD.open("capture0001.bmp", 0x02));
    
    // Stage 4: Read data from parallel port and write to storage
    uint8_t buffer[16];
    uint16_t bytesRead = mockPort.readData(buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL(14, bytesRead); // Size of test data
    
    size_t bytesWritten = mockSD.write(buffer, bytesRead);
    TEST_ASSERT_EQUAL(bytesRead, bytesWritten);
    
    // Stage 5: Complete file storage
    mockSD.sync();
    mockSD.close();
    
    // Stage 6: Update display with completion status
    mockDisplay.updateStatus("Complete");
    TEST_ASSERT_EQUAL_STRING("Complete", mockDisplay.getCurrentMessage());
    
    // Verify file was created with correct data
    TEST_ASSERT_EQUAL(1, mockSD.getFileCount());
    TEST_ASSERT_EQUAL_STRING("capture0001.bmp", mockSD.getFileName(0));
    TEST_ASSERT_EQUAL(14, mockSD.getFileSize(0));
    
    // Verify BMP header detection would work
    TEST_ASSERT_EQUAL(0x42, buffer[0]); // 'B'
    TEST_ASSERT_EQUAL(0x4D, buffer[1]); // 'M'
}