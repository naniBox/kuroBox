@set PATH="C:\ChibiStudio\tools\GNU Tools ARM Embedded\4.7 2013q1\bin";%PATH%
@arm-none-eabi-objdump -t c:\ChibiStudio\workspace\kuroBox\build\kuroBox.elf | gsort -k5
pause