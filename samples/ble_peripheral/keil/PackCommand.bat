@echo off 
::mkdir ..\zephyr 
::copy /Y /B zephyr\.config ..\zephyr\ 
::copy /Y /B zephyr.bin ..\zephyr\ 
mkdir ..\outdir\tai_evb\zephyr 
copy /Y /B build\zephyr.bin ..\outdir\tai_evb\zephyr\ 
..\..\..\..\scripts\kallsyms_win.exe --fbin=ksym.bin < ksym.map > ksym.S 
copy /Y /B ksym.bin ..\zephyr\ 
cd ..\..\..\
python tools\build_fw.py -t -a ble_peripheral -b tai_evb -s tai -p 
