; T5 : line oscillator of the monitor (CTM644, AFC loop of the LA7800). CRTC 1.
; Monitor.cpp follows a model fitted on photos of two sessions that do not agree (README, P2). Each screen
; of this program isolates one unknown of the model ; all the photos must come from the same monitor,
; without touching its H-HOLD.
; CALL &4000,n : screen n for 20 seconds. Mode 2, rows "000102..27" (one byte = 16 Mode 2 pixels), white
; paper, blue border. In a band of lines of the frame :
;   1 : no HSYNC, 64 lines                        free running oscillator : slant = H-HOLD
;   2 : no HSYNC, 200 lines
;   3 : 2nd C-HSYNC of 64 pixels, 464 pixels after the main one, 64 lines  (SHAKER #00BD, #0318)
;   4 : the same, 200 lines
;   5 : 2nd C-HSYNC of 32 pixels, main one of 64, 64 lines
;   6 : 2nd C-HSYNC of 64 pixels, main one of 32, 64 lines
;   7 : 2nd C-HSYNC of 64 pixels, 112 pixels after the main one, 200 lines   does the slide to the left stop ?
;   8 : two more C-HSYNC of 64 pixels, 464 and 640 pixels after the main one, 64 lines  (SHAKER #00C2)
; The bands of 64 lines start 100 lines under the top of the screen, the bands of 200 lines 30 lines under it.
   include 'common.asm'

; Position of the band loop in the line : NOPS before the loop. Set with the emulator so that the first
; I/O of the loop (R3) falls on C0 = 57.
BASE equ 22

; A band of 'lines' lines, 'pre' lines after the interrupt the frame starts on (10 lines above the screen),
; with up to two more HSYNC per line. Each line of the loop, in C0 :
;   57 + shift : R3 = w_extra      5 + shift : R2 = r2_a      16 + shift : R2 = r2_b
;   28 + shift : R2 = r2_c         40 + shift : R3 = w_main
; R3 is never written during a HSYNC : in the band the main HSYNC lasts 6 characters at most (R3 = 6 gives
; the full C-HSYNC of 4 us, R3 = 4 gives 2 us).
macro BAND_SCREEN pre,lines,shift,w_extra,r2_a,r2_b,r2_c,w_main
@frame:                 ; exactly 19968 us
   DELAY_LINES {pre}
   CRTC_WRITE 3,{w_main}   ; 14 : away from the HSYNC
   NOPS 50
   NOPS (BASE+{shift}) % 64
   ld d,{w_extra}       ; 2
   ld e,{r2_a}          ; 2
   ld h,{r2_b}          ; 2
   ld l,{r2_c}          ; 2
   ld a,{w_main}        ; 2
   ld ixl,{lines}       ; 3
@line:                  ; 64 us
   ld bc,#BC03          ; 3
   out (c),c            ; 4
   inc b                ; 1
   out (c),d            ; 4 : R3, width of the next HSYNC
   dec b                ; 1
   ld c,2               ; 2
   out (c),c            ; 4
   inc b                ; 1
   out (c),e            ; 4 : R2
   NOPS 7
   out (c),h            ; 4 : R2
   NOPS 8
   out (c),l            ; 4 : R2
   dec b                ; 1
   ld c,3               ; 2
   out (c),c            ; 4
   inc b                ; 1
   out (c),a            ; 4 : R3, width of the main HSYNC
   dec ixl              ; 2
   jp nz,@line          ; 3
   CRTC_WRITE 2,46      ; 14
   CRTC_WRITE 3,#8E     ; 14
   NOPS 63-((BASE+{shift}) % 64)
   NOPS 4
   if 309-{pre}-{lines} > 255
   DELAY_LINES 255
   DELAY_LINES 309-{pre}-{lines}-255
   else
   DELAY_LINES 309-{pre}-{lines}-1
   DELAY_LINES 1
   endif
   ld hl,(frames)       ; 5 + 2 + 5 + 1 + 1 + 3 = 17
   dec hl
   ld (frames),hl
   ld a,h
   or l
   jp nz,@frame
   jp finish
mend

; A band of 'lines' lines without HSYNC (R3 = 0 : the CRTC 1 sends none)
macro NOSYNC_SCREEN pre,lines
@frame:                 ; exactly 19968 us
   DELAY_LINES {pre}
   NOPS (BASE+30) % 64
   CRTC_WRITE 3,#80     ; 14
   DELAY_LINES {lines}
   CRTC_WRITE 3,#8E     ; 14
   NOPS 63-((BASE+30) % 64)
   NOPS 16
   if 310-{pre}-{lines} > 255
   DELAY_LINES 255
   DELAY_LINES 310-{pre}-{lines}-255
   else
   DELAY_LINES 310-{pre}-{lines}-1
   DELAY_LINES 1
   endif
   ld hl,(frames)       ; 17
   dec hl
   ld (frames),hl
   ld a,h
   or l
   jp nz,@frame
   jp finish
mend

   org #4000
start:
   or a                 ; CALL &4000,n : A = number of parameters, DE = n
   ld a,3
   jr z,@default
   ld a,e
@default:
   dec a                ; 1 to 8, 3 otherwise
   cp 8
   jr c,@known
   ld a,2
@known:
   add a,a
   ld e,a
   ld d,0
   ld hl,screens
   add hl,de
   ld e,(hl)
   inc hl
   ld d,(hl)
   ld (entry),de
   ld a,2
   call #BC0E           ; mode 2
   ld b,25              ; 25 rows "000102..27" ; the last character of the screen is left out (no scroll)
@row:
   ld c,0
@byte:
   ld a,c
   rrca
   rrca
   rrca
   rrca
   call digit
   ld a,b
   dec a
   jr nz,@low
   ld a,c
   cp 39
   jr z,@next
@low:
   ld a,c
   call digit
@next:
   inc c
   ld a,c
   cp 40
   jr nz,@byte
   djnz @row
   di
   ld bc,#7F8E          ; mode 2, ROMs off
   out (c),c
   ld bc,#7F00          ; paper : white
   out (c),c
   ld bc,#7F4B
   out (c),c
   ld bc,#7F01          ; pen : black
   out (c),c
   ld bc,#7F54
   out (c),c
   ld bc,#7F10          ; border : blue
   out (c),c
   ld bc,#7F44
   out (c),c
   ld a,(#0038)         ; interrupt : comes back at once, interrupts off
   ld (saved),a
   ld a,#C9
   ld (#0038),a
   ld hl,1000
   ld (frames),hl
   WAIT_VSYNC_RISE      ; line 240
   DELAY_LINES 10
   ld bc,#7F9E          ; interrupt counter of the GATE ARRAY to 0, no interrupt waiting : the next one
   out (c),c            ; comes at the end of the 52nd HSYNC from here, 10 lines before the top of the screen
   ei
   halt                 ; the frame loop starts on a known microsecond of the line
   ld hl,(entry)        ; the same time for every screen
   jp (hl)

screen1: NOSYNC_SCREEN 110,64
screen2: NOSYNC_SCREEN 40,200
screen3: BAND_SCREEN 110,64,0,#86,11,46,46,#86
screen4: BAND_SCREEN 40,200,0,#86,11,46,46,#86
screen5: BAND_SCREEN 110,64,0,#84,11,46,46,#86
screen6: BAND_SCREEN 110,64,0,#86,11,46,46,#84
screen7: BAND_SCREEN 40,200,22,#86,46,46,53,#86
screen8: BAND_SCREEN 110,64,0,#86,11,22,46,#86

finish:
   ld a,(saved)
   ld (#0038),a
   BACK_TO_BASIC

digit:                  ; prints the low digit of A in hexadecimal
   and 15
   add a,#90
   daa
   adc a,#40
   daa
   jp #BB5A
entry: dw 0
screens: dw screen1,screen2,screen3,screen4,screen5,screen6,screen7,screen8
saved: db 0
frames: dw 0
end:
   save 'T5MON.BIN',start,end-start,DSK,'realcpc.dsk'
