; Shared core of the real CPC test programs (README.md, part 1) : CRTC.BIN, GA.BIN, MONITEUR.BIN, FDC.BIN on
; realcpc.dsk. Each program includes common.asm, sets org #4000, includes this file and its tests, and defines :
; menu_name (title text), menu_list (prints the list of its tests), entries (key, test, parameter, result page).
; The menu shows the CRTC found (compendium 28.1.9). A screen test runs until a key is pressed ; a measure shows its
; result page, then a key brings the menu back. calib.asm : AUTO_TEST runs again and again, no menu.
start:
   ifdef AUTO
auto_loop:
   call AUTO_TEST
   jr auto_loop
   endif
   call detect_crtc
   ld (crtc_type),a
menu:
   call show_menu
.read:
   call #BB06           ; KM WAIT CHAR
   cp 'a'
   jr c,.upper
   sub 32               ; lower case letters
.upper:
   ld c,a
   ld hl,entries
.find:
   ld a,(hl)
   or a
   jr z,.read           ; not a key of the menu
   cp c
   jr z,.found
   ld de,6
   add hl,de
   jr .find
.found:
   inc hl
   ld e,(hl)
   inc hl
   ld d,(hl)
   inc hl
   ld a,(hl)
   ld (param),a
   inc hl
   ld a,(hl)
   inc hl
   ld h,(hl)
   ld l,a
   ld (show_addr),hl
   ld (test_addr),de
   call run_test
   ld hl,(show_addr)    ; result page, if any
   ld a,h
   or l
   jp z,menu
   ld a,1
   call #BC0E           ; SCR SET MODE (HL is lost)
   ld hl,(show_addr)
   ld de,menu
   push de              ; the page returns to the menu once a key is pressed
   ld de,result_wait
   push de
   jp (hl)
result_wait:
   ld hl,txt_key
   call print_str
   jp #BB06             ; KM WAIT CHAR

; Runs the test at test_addr with A = param, once the key of the menu is up, then puts the firmware back
run_test:
   di
   call wait_release
   ei
   ld a,(param)
   ld hl,(test_addr)
   call .jump
   di
   CRTC_WRITE 0,63      ; standard CRTC values the tests change
   CRTC_WRITE 1,40
   CRTC_WRITE 2,46
   CRTC_WRITE 3,#8E
   CRTC_WRITE 4,38
   CRTC_WRITE 7,30
   CRTC_WRITE 9,7
   ld bc,#7F85          ; mode 1, ROMs as the firmware expects them
   out (c),c
   call wait_release
   im 1
   ei
   call #BB03           ; KM RESET : no key left from the test
   call #BC02           ; SCR RESET : the inks of the firmware
   ret
.jump:
   jp (hl)

quit:
   rst 0

; Waits until no key is down for two frames. Interrupts off (the firmware scans the keyboard too).
wait_release:
   call keyscan
   ld a,(key)
   or a
   jr nz,wait_release
   WAIT_VSYNC_RISE
   WAIT_VSYNC_RISE
   call keyscan
   ld a,(key)
   or a
   jr nz,wait_release
   ret

; key = #00 if no key is down, not #00 otherwise (the 10 lines of the keyboard, joystick included).
; Exactly 256 us with the CALL and the RET, whatever the keys : it replaces a DELAY_LINES 4 in the frame loops.
; Keeps every register.
macro KEY_LINE line
   ld bc,#F640+{line}   ; 3 : PSG read, keyboard line
   out (c),c            ; 4
   ld b,#F4             ; 2
   in a,(c)             ; 4 : PPI port A, 0 = key down
   and d                ; 1
   ld d,a               ; 1
mend
keyscan:                ; CALL : 5
   push af              ; 4
   push bc              ; 4
   push de              ; 4
   ld d,#FF             ; 2
   ld bc,#F40E          ; 7 : PSG register 14
   out (c),c
   ld bc,#F6C0          ; 7
   out (c),c
   ld bc,#F600          ; 7
   out (c),c
   ld bc,#F792          ; 7 : PPI port A input
   out (c),c
   KEY_LINE 0           ; 10 x 15
   KEY_LINE 1
   KEY_LINE 2
   KEY_LINE 3
   KEY_LINE 4
   KEY_LINE 5
   KEY_LINE 6
   KEY_LINE 7
   KEY_LINE 8
   KEY_LINE 9
   ld bc,#F782          ; 7 : PPI as the firmware expects it
   out (c),c
   ld bc,#F600          ; 7
   out (c),c
   ld a,d               ; 1
   cpl                  ; 1
   ld (key),a           ; 4
   pop de               ; 3
   pop bc               ; 3
   pop af               ; 3
   NOPS 27
   ret                  ; 3 : 256

; A = '0', '1', '2' or '3' (CRTC 3 or 4), read on the port &BF00 (compendium 28.1.9) :
; R12 reads 0 on the CRTC 1 and 2, its value (#30 under the firmware) on the CRTC 0, 3 and 4 ;
; R4 reads 0 on the CRTC 0, R12 on the CRTC 3 and 4 (3 bits of register number) ;
; R31 reads not 0 on the CRTC 1 (255 or 127 seen), 0 on the CRTC 2.
detect_crtc:
   ld bc,#BC0C
   out (c),c
   ld b,#BF
   in a,(c)
   or a
   jr z,.one_or_two
   ld e,a
   ld bc,#BC04
   out (c),c
   ld b,#BF
   in a,(c)
   cp e
   ld a,'0'
   ret nz
   ld a,'3'
   ret
.one_or_two:
   ld bc,#BC1F
   out (c),c
   ld b,#BF
   in a,(c)
   or a
   ld a,'1'
   ret nz
   ld a,'2'
   ret

show_menu:
   ld a,1
   call #BC0E           ; SCR SET MODE : mode 1, screen cleared
   ld hl,menu_name
   call print_str
   ld hl,txt_detected
   call print_str
   ld a,(crtc_type)
   cp '3'
   jr z,.three_four
   call #BB5A
   jr .list
.three_four:
   ld hl,txt_three_four
   call print_str
.list:
   ld hl,txt_newline
   call print_str
   call menu_list
   ld hl,txt_help
   jp print_str

; Header of the section of CRTC A (text HL) : pen 3 and a mark on the CRTC found, pen 2 otherwise
section:
   ld b,a
   ld a,(crtc_type)
   cp b
   push af
   ld a,15
   call #BB5A
   pop af
   push af
   ld a,2
   jr nz,.pen
   ld a,3
.pen:
   call #BB5A
   call print_str
   pop af
   ld hl,txt_found
   call z,print_str
   ld hl,txt_newline
   jp print_str

print_char:                 ; A on the screen (firmware TXT OUTPUT, keeps every register)
   jp #BB5A

print_str:                  ; text HL ending with 0
   ld a,(hl)
   or a
   ret z
   call #BB5A
   inc hl
   jr print_str

print_dec:              ; HL in decimal, no leading zeros
   ld e,0               ; 1 once a digit is printed
   ld bc,-10000
   call .digit
   ld bc,-1000
   call .digit
   ld bc,-100
   call .digit
   ld bc,-10
   call .digit
   ld e,1
   ld bc,-1
.digit:
   ld a,'0'-1
.sub:
   inc a
   add hl,bc
   jr c,.sub
   sbc hl,bc
   cp '0'
   jr nz,.show
   bit 0,e
   ret z
.show:
   ld e,1
   jp #BB5A

print_hex:              ; A in hexadecimal, 2 digits
   push af
   rrca
   rrca
   rrca
   rrca
   call .digit
   pop af
.digit:
   and 15
   add a,#90
   daa
   adc a,#40
   daa
   jp #BB5A

txt_detected:   db 13,10,"CRTC DETECTE : ",0
txt_three_four: db "3 OU 4",0
txt_found:     db " <== CE CPC",0
txt_newline:   db 13,10,0
txt_help:      db 13,10,15,1,"UNE TOUCHE ARRETE L'ECRAN.  Q : BASIC",0
txt_key:       db 13,10,13,10,15,2,"UNE TOUCHE : MENU",0

crtc_type: db 0
key:       db 0
param:     db 0
test_addr: dw 0
show_addr: dw 0
