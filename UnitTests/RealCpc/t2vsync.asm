; T2VSYNC : a VSYNC restarted after an odd number of HSYNC. CRTC 1 only : its VSYNC lasts 16 lines, and it starts
; as soon as R7 = C4.
; 40010-simplified, sheet VSync : the rising edge of the VSYNC resets the bits 1 to 4 of HCNT, not the bit 0 (U809).
; R52 goes back to 0 when HCNT reaches 4 : after 2 HSYNC, or after 1 if the bit 0 stayed at 1.
; A first VSYNC, then a second one started 17 (odd) or 18 (even) HSYNC later, R52 = 0 then 52 lines to the
; interrupt. Time from the second VSYNC to that interrupt, in steps of 10 us (6.4 steps = 1 line).
; t2_odd : 17 HSYNC, t2_even : 18 HSYNC.
; Both about the same (about 345) : the bit 0 has no effect. The first about 6 less : it has one.
; Included by ga.asm. The IM 2 table is built at #9000 (257 bytes #91) and the interrupt code, which sets t2_flag,
; copied to #9191 : the vector is #9191 whatever the data bus.
t2_run:
   di
   ld hl,#9000          ; IM 2 table
   ld de,#9001
   ld bc,256
   ld (hl),#91
   ldir
   ld hl,t2_interrupt   ; its code at #9191
   ld de,#9191
   ld bc,t2_interrupt_end-t2_interrupt
   ldir
   im 2
   ld a,#90
   ld i,a
   ld a,18              ; the first measure is not stable (R52 before the first VSYNC) : not kept
   call t2_measure
   ld a,17
   call t2_measure
   ld (t2_odd),de
   ld a,18
   call t2_measure
   ld (t2_even),de
   di
   im 1
   ret

; A = lines from the first VSYNC to the second one
t2_measure:
   ld (t2_lines),a
   WAIT_VSYNC_RISE      ; line 240 of the frame, C4 = 30, C9 = 0
   DELAY_LINES 3
   ei                   ; the interrupt of the 2nd HSYNC of the VSYNC, if any
   nop
   nop
   di
   ld a,(t2_lines)
   sub 4                ; the loop runs A + 1 times
   ld d,a
t2_wait_lines:          ; 64 us
   NOPS 58
   ld a,d
   dec d
   or a
   jr nz,t2_wait_lines
   xor a
   ld (t2_flag),a
   CRTC_WRITE 7,32      ; C4 = 32 on these lines : the second VSYNC
   ld de,0
   ei
t2_count:               ; 10 us
   inc de
   ld a,(t2_flag)
   or a
   jr z,t2_count
   di
   CRTC_WRITE 7,30
   ret

t2_interrupt:           ; copied to #9191
   push af
   ld a,1
   ld (t2_flag),a
   pop af
   ei
   ret
t2_interrupt_end:

t2_lines: db 0
t2_flag:  db 0
t2_odd:   dw 0
t2_even:  dw 0
