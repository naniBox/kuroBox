REM @echo off
c:
cd c:\ChibiStudio\workspace\kuroBox\scripts\
set PYTHONPATH=c:\ChibiStudio\workspace\kuroBox\scripts\
python kurobox/kbb_viewer.py %1
REM start pythonw kbb_viewer.py %1
pause
