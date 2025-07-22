@ECHO OFF
CLS
REM taskkill /IM putty.exe /F >nul 2>&1
REM timeout /t 1 >nul
REM -t upload 
echo #### START DEPLOYMENT ##########################################################################################
echo "pio run -e megaatmega2560 --target upload"
pio run -e megaatmega2560 --target upload --verbose
echo #### END   DEPLOYMENT ##########################################################################################
REM putty.exe -serial COM3 -sercfg 115200,8,n,1,N
