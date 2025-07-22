"%USERPROFILE%\.platformio\packages\tool-avrdude\avrdude.exe" ^
    -C "%USERPROFILE%\.platformio\packages\tool-avrdude\avrdude.conf" ^
    -v ^
    -p atmega2560 ^
    -c wiring ^
    -P COM3 ^
    -b 115200 ^
    -D ^
    -U flash:r:firmware.bin:r
