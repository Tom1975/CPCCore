; T2VSYNC : a VSYNC restarted after an odd number of HSYNC. CRTC 1 only : its VSYNC lasts 16 lines, and it starts
; as soon as R7 = C4.
; 40010-simplified, sheet VSync : the rising edge of the VSYNC resets the bits 1 to 4 of HCNT, not the bit 0 (U809).
; R52 goes back to 0 when HCNT reaches 4 : after 2 HSYNC, or after 1 if the bit 0 stayed at 1.
; A first VSYNC, then a second one started 17 (odd) or 18 (even) HSYNC later, R52 = 0 then 52 lines to the
; interrupt. Time from the second VSYNC to that interrupt, in steps of 10 us (6.4 steps = 1 line).
; #4F00 (word) : 17 HSYNC, #4F02 (word) : 18 HSYNC.
; Both about the same (about 345) : the bit 0 has no effect. The first about 6 less : it has one.
   include 'common.asm'
   org #4000
start:
   di
   im 2
   ld a,#60
   ld i,a
   ld a,18              ; the first measure is not stable (R52 before the first VSYNC) : not kept
   call measure
   ld a,17
   call measure
   ld (#4F00),de
   ld a,18
   call measure
   ld (#4F02),de
   BACK_TO_BASIC

; A = lines from the first VSYNC to the second one
measure:
   ld (lines),a
   WAIT_VSYNC_RISE      ; line 240 of the frame, C4 = 30, C9 = 0
   DELAY_LINES 3
   ei                   ; the interrupt of the 2nd HSYNC of the VSYNC, if any
   nop
   nop
   di
   ld a,(lines)
   sub 4                ; the loop runs A + 1 times
   ld d,a
wait_lines:             ; 64 us
   NOPS 58
   ld a,d
   dec d
   or a
   jr nz,wait_lines
   xor a
   ld (flag),a
   CRTC_WRITE 7,32      ; C4 = 32 on these lines : the second VSYNC
   ld de,0
   ei
count:                  ; 10 us
   inc de
   ld a,(flag)
   or a
   jr z,count
   di
   CRTC_WRITE 7,30
   ret

lines: db 0
flag:  db 0

   org #6000            ; IM 2 table, the vector is #6161 whatever the data bus
   repeat 257
   db #61
   rend
   org #6161
   push af
   ld a,1
   ld (flag),a
   pop af
   ei
   ret
end:
   save 'T2VSYNC.BIN',start,end-start,DSK,'realcpc.dsk'
