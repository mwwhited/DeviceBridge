@ECHO OFF

REM taskkill /IM putty.exe /F >nul 2>&1
REM timeout /t 1 >nul
REM -t upload 
echo #### START TESTS ##########################################################################################
echo "pio test -vvv -e megaatmega2560_test"
pio test -vvv -e megaatmega2560_test
echo #### END   TESTS ##########################################################################################
REM putty.exe -serial COM3 -sercfg 115200,8,n,1,N