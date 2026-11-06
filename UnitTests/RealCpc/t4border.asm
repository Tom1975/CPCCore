; T4BORDER : do the border and the inks change at the same position ?
; 40010-simplified, sheet Registers, U401 : the same strobe for both. Compendium 9.2 : no difference.
; Pen 0 goes from red to black with OUT (C),r on the 8 last lines of the screen, the border on the 8 first lines
; under the screen, at the same microsecond of the line. Two red bands, one above the other.
; Right ends aligned : same position. 8 Mode 2 pixels apart : the border is one byte early. 15 seconds.
   include 'common.asm'
   org #4000
start:
   di
   ld hl,#C000
   ld de,#C001
   ld bc,#3FFF
   ld (hl),0
   ldir
   ld hl,750
   ld (frames),hl
   ld d,#54             ; black
   ld e,#4C             ; bright red
   WAIT_VSYNC_RISE      ; line 240
   ld b,#7F
frame:                  ; exactly 19968 us
   DELAY_LINES 255      ; 2 delays, 64 * 264 + 2 : line 192
   DELAY_LINES 9
   ld hl,pens           ; 3
line_loop:              ; 64 us
   ld a,(hl)            ; 2
   inc hl               ; 2
   out (c),a            ; 4 : pen
   out (c),d            ; 4 : black
   NOPS 20
   out (c),e            ; 4 : red
   NOPS 22
   ld a,l               ; 1
   cp pens_end and #FF  ; 2
   jr nz,line_loop      ; 3
   ld a,#10             ; 23 : both black again, at the same position as in the lines above
   nop
   nop
   nop
   out (c),a
   out (c),d
   xor a
   nop
   out (c),a
   out (c),d
   DELAY_LINES 31
   NOPS 19
   ld hl,(frames)       ; 17
   dec hl
   ld (frames),hl
   ld a,h
   or l
   jp nz,frame
   BACK_TO_BASIC
frames: dw 0
   align 256
pens:
   db 0,0,0,0,0,0,0,0,#10,#10,#10,#10,#10,#10,#10,#10
pens_end:
end:
   save 'T4BORDER.BIN',start,end-start,DSK,'realcpc.dsk'
