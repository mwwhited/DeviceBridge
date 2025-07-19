#include <unity.h>
#include <Arduino.h>

// Helper function to calculate free RAM
int freeRam() {
    extern int __heap_start, *__brkval;
    int v;
    return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}

void test_basic() {
    TEST_ASSERT_EQUAL(4, 2 + 2);
    Serial.println(F("Basic test OK"));
}

void test_memory() {
    int ram = freeRam();
    Serial.print(F("RAM: "));
    Serial.println(ram);
    TEST_ASSERT_TRUE(ram > 5000);
}

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println(F("Test Start"));
    
    UNITY_BEGIN();
    RUN_TEST(test_basic);
    RUN_TEST(test_memory);
    UNITY_END();
    
    Serial.println(F("Test Complete"));
    Serial.println(F("Button monitoring:"));
}

void loop() {
    static unsigned long last = 0;
    if (millis() - last > 2000) {
        last = millis();
        
        uint16_t btn = analogRead(A0);
        Serial.print(F("A0="));
        Serial.print(btn);
        
        if (btn < 50) Serial.print(F(" RIGHT"));
        else if (btn < 200) Serial.print(F(" UP"));
        else if (btn < 400) Serial.print(F(" DOWN"));
        else if (btn < 600) Serial.print(F(" LEFT"));
        else if (btn < 800) Serial.print(F(" SELECT"));
        else Serial.print(F(" NONE"));
        
        Serial.print(F(" RAM="));
        Serial.println(freeRam());
    }
}