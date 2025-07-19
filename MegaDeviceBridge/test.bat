REM taskkill /IM putty.exe /F >nul 2>&1
REM timeout /t 1 >nul
pio test -vvv -t upload -e megaatmega2560_test
REM putty.exe -serial COM3 -sercfg 115200,8,n,1,N