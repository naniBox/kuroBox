@set PATH=%PATH%;c:\ChibiStudio\tools\openocd\bin\
@REM -d 3
openocd.exe -f scripts\board\stm32f4discovery.cfg
@pause