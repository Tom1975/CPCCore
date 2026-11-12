; T3MODE : does a R3.JIT move the mode change ?
; 40010-simplified, sheet VideoControl : U1003, U1004 are flip-flops on the end of SIG_GA_HSYNC : the mode would
; change 4 Mode 2 pixels later with a R3.JIT. Compendium 9.3.4.4 : only the end of the black moves. The emulation
; follows the compendium (a latch open during SIG_GA_HSYNC).
; Screen of #B4 in mode 2, R1 = 63 : the bytes under the HSYNC are displayed. 12 rows of 16 lines. In each row, 4
; times : mode 0 asked, R3 = 3, then R3 = 2 written 1 us later than in the row above, then mode 2 asked. It runs
; over the frame positions around the 3rd microsecond of the HSYNC : compare the rows with the capture of Sugarbox,
; zoomed, where the image restarts after the HSYNC. 15 seconds.
   include 'common.asm'
   org #4000
start:
   di
   ld hl,#C000
   ld de,#C001
   ld bc,#3FFF
   ld (hl),#B4
   ldir
   ld bc,#7F8E          ; mode 2, ROMs off
   out (c),c
   CRTC_WRITE 1,63
   ld hl,750
   ld (frames),hl
   WAIT_VSYNC_RISE      ; line 240
frame:                  ; exactly 19968 us
   DELAY_LINES 76       ; 64 * 76 + 1 : line 4
   for_row = 0
   repeat 12
   ld e,4               ; a row : 2 + 4 * 128 - 1 + 64 * 7 + 1 + 62 = 1024 us
@pair:                  ; 128 us
   ld bc,#7F8C          ; 7 : mode 0
   out (c),c
   ld bc,#BC03          ; 7
   out (c),c
   ld bc,#BD83          ; 7 : R3 = 3
   out (c),c
   NOPS 14+for_row
   ld bc,#BD82          ; 7 : R3 = 2, I/O on the 3rd us of the OUT
   out (c),c
   NOPS 22-for_row
   ld bc,#7F8E          ; 7 : mode 2
   out (c),c
   ld bc,#BD8E          ; 7 : R3 back
   out (c),c
   NOPS 46
   dec e                ; 4
   jr nz,@pair
   DELAY_LINES 7
   NOPS 62
   for_row = for_row + 1
   rend
   DELAY_LINES 43
   NOPS 45
   ld hl,(frames)       ; 5 + 2 + 5 + 1 + 1 + 3 = 17
   dec hl
   ld (frames),hl
   ld a,h
   or l
   jp nz,frame
   BACK_TO_BASIC
frames: dw 0
end:
   save 'T3MODE.BIN',start,end-start,DSK,'realcpc.dsk'
