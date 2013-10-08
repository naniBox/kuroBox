@set PATH=%PATH%;"C:\Program Files (x86)\SEGGER\JLinkARM_V470a\"
c:
cd "c:\ChibiStudio\workspace\kuroBox\"
jlink -CommanderScript FLASH_kuroBox.jlink
pause
