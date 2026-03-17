@echo off

@echo --== flash app.bin ==--
pyocd flash -t atb111x -a 0x2200 app.bin

pause