kuroBox
=======

This is the software layer for naniBox.com's kuroBox hardware v11. At its
core, it's a datalogger with a status screen, a couple of buttons, an Linear
Time Code (LTC) front-end, a couple of RS232 ports and an expansion header. 

Data can come from various sources, such as the LTC input, RS232 ports or
through SPI/I2C/USART via the expansion header. 

Data is written to SD card at a default rate of 200Hz. The data is a arranged 
in a 512byte datablock, where all of it gets written out. The separate data
sources have slots in this datablock and update their slots based on their
respective update rates. This means that slowly updating information will
be repeated often, but it also means that each datablock is atomic, independant
of all prior data, making it easy to quickly discard irrelevant data.

The hardware is not opensource, contact me at dmo@nanibox.com if you want
more details.

Licenses
--------

This software uses the following pieces of software, which have their own 
licensing schemes, check them to make sure you know what you're getting into.

ChibiOS: 
http://chibios.org/

FatFS:
http://elm-chan.org/fsw/ff/00index_e.html

LTC: 
Original was: https://github.com/x42/libltc
But I forked it for MCU-specific modifications: https://github.com/talsit/libltc

