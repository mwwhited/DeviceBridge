
REM taskkill /IM putty.exe /F >nul 2>&1
REM timeout /t 1 >nul

pio run -e megaatmega2560
REM pio run -e esp32dev
REM pio run -e bluepill

REM putty.exe -serial COM3 -sercfg 115200,8,n,1,N