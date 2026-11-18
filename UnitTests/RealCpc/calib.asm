; Calibration build : calib.asm AUTO (1 : P1, 2 : P8, 3 : FDC) -> calib.dsk, the test in a loop, no menu.
; rasm calib.asm -eo, after setting AUTO below. Not on realcpc.dsk.
AUTO equ 1
   include 'menu.asm'
