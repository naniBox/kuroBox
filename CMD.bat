@echo off
rem This file is the script to set path for ARM eabi tool chain.

@set PATH="C:\ChibiStudio\tools\GNU Tools ARM Embedded\4.7 2013q1\bin";"c:\Program Files (x86)\SEGGER\JLinkARM_V470a\";%PATH%
cmd.exe /K cd "c:\ChibiStudio\workspace\kuroBox\"
