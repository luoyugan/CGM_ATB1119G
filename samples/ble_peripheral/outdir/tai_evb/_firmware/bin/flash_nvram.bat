@echo off

@echo --== flash nvram.bin ==--
pyocd flash -t atb111x -a 0x70000 nvram.bin

pause