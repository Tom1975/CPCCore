; Shared by the real CPC tests. RASM : hexadecimal is #, & is AND.
; A Z80 instruction on the CPC takes a whole number of microseconds (NOPs).

; 64 * lines + 1 microseconds. Uses A. Lines : 1 to 255.
macro DELAY_LINES lines
   ld a,{lines}
@line
   repeat 60
   nop
   rend
   dec a
   jr nz,@line
mend

macro NOPS count
   repeat {count}
   nop
   rend
mend

; Waits for the rising edge of the VSYNC (PPI port B bit 0), 8 us of uncertainty. Uses A, B.
macro WAIT_VSYNC_RISE
   ld b,#F5
@low
   in a,(c)
   rra
   jr c,@low
@high
   in a,(c)
   rra
   jr nc,@high
mend

; CRTC register reg = value. Uses BC.
macro CRTC_WRITE reg,value
   ld bc,#BC00+{reg}
   out (c),c
   ld bc,#BD00+{value}
   out (c),c
mend

; Back to BASIC : standard CRTC values, ROMs as the firmware expects them, mode 1, screen cleared.
macro BACK_TO_BASIC
   CRTC_WRITE 1,40
   CRTC_WRITE 3,#8E
   CRTC_WRITE 7,30
   ld bc,#7F85
   out (c),c
   im 1
   ei
   ld a,1
   call #BC0E
   ret
mend
