@echo off

@echo --== flash erase ==--
pyocd erase --sector 0x0000-0x80000 -t atb111x

@echo --== flash nvram.bin ==--
pyocd flash -t atb111x -a 0x70000 nvram.bin

@echo --== flash mbrec.bin ==--
pyocd flash -t atb111x -a 0x0000 mbrec.bin

@echo --== flash app.bin ==--
pyocd flash -t atb111x -a 0x2200 app.bin

pause