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

; End of a frame loop of a test : 14 us, then a JP Z,frame (3 us) back to the frame while no key is down (the flag
; 'key' is set by the 'call keyscan' of the frame) : 17 us as the frame counter it replaces. Uses A.
macro KEY_UP_TEST
   ld a,(key)           ; 4
   or a                 ; 1
   NOPS 9
mend
