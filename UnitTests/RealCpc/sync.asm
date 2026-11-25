; Frame synchronisation on a known microsecond, shared by P1 and P8. Included by crtc.asm.
; sync_frame : with the firmware CRTC values (312 lines, VSYNC on line 240), returns on a fixed microsecond of the
; frame, interrupts off : the GATE ARRAY interrupt counter is reset 10 lines after the VSYNC, its interrupt comes at
; the end of the 52nd HSYNC from there, and HALT leaves on it (the code at #0038 is a RET while int_patch is on).
; The microsecond is set with the emulator : see the positions in p1last.asm, p8vsync.asm.

int_patch:              ; #0038 = RET, ROMs off (the RAM under the lower ROM is read)
   di
   ld bc,#7F8D          ; mode 1, ROMs off
   out (c),c
   ld hl,#0038          ; the 4 bytes at #0038 (synctest.asm writes a POP HL, JP there)
   ld de,int_saved
   ld bc,4
   ldir
   ld a,#C9
   ld (#0038),a
   ret

int_restore:
   di
   ld hl,int_saved
   ld de,#0038
   ld bc,4
   ldir
   ret

sync_frame:
   di
   WAIT_VSYNC_RISE      ; line 240
   DELAY_LINES 10
   ld bc,#7F9D          ; interrupt counter to 0 (mode 1, ROMs off), no interrupt waiting
   out (c),c
   ei
   halt
   ret                  ; the same microsecond every time

; DE = number of 10 us steps until the VSYNC rises. Uses A, B.
count_to_vsync_rise:
   ld b,#F5
   ld de,0
.low:
   inc de               ; 2
   in a,(c)             ; 4
   rra                  ; 1
   jr nc,.low           ; 3
   ret

; DE = number of 10 us steps until the VSYNC falls (waiting for it first if it is not there yet). Uses A, B.
count_to_vsync_fall:
   ld b,#F5
   ld de,0
.low:
   inc de               ; 2
   in a,(c)             ; 4
   rra                  ; 1
   jr nc,.low           ; 3
.high:
   inc de               ; 2
   in a,(c)             ; 4
   rra                  ; 1
   jr c,.high           ; 3
   ret

int_saved: ds 4
