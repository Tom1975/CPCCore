; Calibration build : the test AUTO_TEST runs again and again, no menu -> calib.dsk (not on realcpc.dsk).
; rasm calib.asm -eo, after choosing the test below : p1_run, p8_run or sync_run (crtc.asm), fdc_run (fdc.asm).
AUTO equ 1
AUTO_TEST equ p1_run
   include 'crtc.asm'
