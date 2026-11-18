; MENU : the real CPC tests of README.md, part 1. RUN"MENU" from realcpc.dsk (built by build.sh).
; The menu shows the CRTC found (compendium 28.1.9) and the tests sorted by CRTC type. A screen test runs until a key
; is pressed ; a measure shows its result page, then a key brings the menu back.
   include 'common.asm'
   org #4000
   run start
start:
   ifdef AUTO
   jp auto_loop
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
   ld hl,txt_title
   call print_str
   ld a,(crtc_type)
   cp '3'
   jr z,.three_four
   call #BB5A
   jr .sections
.three_four:
   ld hl,txt_three_four
   call print_str
.sections:
   ld hl,txt_all
   call print_str
   ld hl,txt_tests_all
   call print_str
   ld a,'0'
   ld hl,txt_crtc0
   call section
   ld a,'1'
   ld hl,txt_crtc1
   call section
   ld hl,txt_tests1
   call print_str
   ld a,'2'
   ld hl,txt_crtc2
   call section
   ld a,'3'
   ld hl,txt_crtc34
   call section
   ld hl,txt_help
   jp print_str

t1_show:
   ld hl,txt_res_t1
   call print_str
   ld a,(t1_value)
   ld l,a
   ld h,0
   call print_dec
   ld hl,txt_res_t1b
   jp print_str

t2_show:
   ld hl,txt_res_t2
   call print_str
   ld hl,(t2_odd)
   call print_dec
   ld hl,txt_res_t2b
   call print_str
   ld hl,(t2_even)
   jp print_dec

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

   include 'sync.asm'
   include 't1in.asm'
   include 't2vsync.asm'
   include 't3mode.asm'
   include 't4border.asm'
   include 't5mon.asm'
   include 'p1last.asm'
   include 'p8vsync.asm'
   include 'fdcrqm.asm'

; Calibration with the emulator (calib.asm) : the test AUTO runs again and again, no menu
   ifdef AUTO
auto_loop:
   if AUTO == 1
   call p1_run
   endif
   if AUTO == 2
   call p8_run
   endif
   if AUTO == 3
   call fdc_run
   endif
   jr auto_loop
   endif

; key, test, parameter (A), result page (0 : none)
entries:
   db '1' : dw t1_run : db 0 : dw t1_show
   db '2' : dw t4_run : db 0 : dw 0
   db '3' : dw fdc_run : db 0 : dw fdc_show
   db '4' : dw t2_run : db 0 : dw t2_show
   db '5' : dw t3_run : db 0 : dw 0
   db '6' : dw p1_run : db 0 : dw p1_show
   db '7' : dw p8_run : db 0 : dw p8_show
   db '8' : dw t5_run : db 1 : dw 0
   db '9' : dw t5_run : db 2 : dw 0
   db 'A' : dw t5_run : db 3 : dw 0
   db 'B' : dw t5_run : db 4 : dw 0
   db 'C' : dw t5_run : db 5 : dw 0
   db 'D' : dw t5_run : db 6 : dw 0
   db 'E' : dw t5_run : db 7 : dw 0
   db 'F' : dw t5_run : db 8 : dw 0
   db 'Q' : dw quit : db 0 : dw 0
   db 0

; Texts : 15,n = pen n ; 13,10 = new line. 39 characters at most per line.
txt_title:     db 15,1,"TESTS SUR VRAI CPC",13,10,"CRTC DETECTE : ",0
txt_three_four: db "3 OU 4",0
txt_all:       db 13,10,13,10,15,2,"TOUS LES CRTC",13,10,0
txt_tests_all: db 15,1," 1  T1  IN &74xx : PPI ET GATE ARRAY",13,10
               db " 2  T4  BORDER ET ENCRE",13,10
               db " 3  FDC RQM ENTRE OCTETS DE RESULTAT",13,10,0
txt_crtc0:     db "CRTC 0 : AUCUN TEST",0
txt_crtc1:     db "CRTC 1",0
txt_tests1:    db 15,1," 4  T2  VSYNC APRES 17 OU 18 HSYNC",13,10
               db " 5  T3  MODE ET R3.JIT",13,10
               db " 6  P1  ECHANTILLON DERNIERE LIGNE",13,10
               db " 7  P8  COMPTEUR DE LIGNES VSYNC",13,10
               db " 8  T5  MONITEUR 1 : 64 L SANS HSYNC",13,10
               db " 9  T5  MONITEUR 2 : 200 L SANS HSYNC",13,10
               db " A  T5  MONITEUR 3 : HSYNC +464, 64 L",13,10
               db " B  T5  MONITEUR 4 : HSYNC +464, 200 L",13,10
               db " C  T5  MONITEUR 5 : 2E HSYNC 32 PX",13,10
               db " D  T5  MONITEUR 6 : PRINCIPALE 32 PX",13,10
               db " E  T5  MONITEUR 7 : HSYNC +112, 200 L",13,10
               db " F  T5  MONITEUR 8 : 2 HSYNC EN PLUS",13,10,0
txt_crtc2:     db "CRTC 2 : AUCUN TEST",0
txt_crtc34:    db "CRTC 3 ET 4 : AUCUN TEST",0
txt_found:     db " <== CE CPC",0
txt_newline:   db 13,10,0
txt_help:      db 13,10,15,1,"UNE TOUCHE ARRETE L'ECRAN.  Q : BASIC",0
txt_key:       db 13,10,13,10,15,2,"UNE TOUCHE : MENU",0
txt_res_t1:    db 15,2,"T1 : IN &7400",13,10,13,10,15,1,"VALEUR LUE ",0
txt_res_t1b:   db " (76 ATTENDU)",0
txt_res_t2:    db 15,2,"T2 : VSYNC APRES 17 OU 18 HSYNC",13,10,13,10,15,1,"17 HSYNC : ",0
txt_res_t2b:   db 13,10,"18 HSYNC : ",0

crtc_type: db 0
key:       db 0
param:     db 0
test_addr: dw 0
show_addr: dw 0
   assert $ <= #9000    ; T2 builds its IM 2 table at #9000 (t2vsync.asm)

end:
   ifdef AUTO
   save 'MENU.BIN',start,end-start,DSK,'calib.dsk'
   else
   save 'MENU.BIN',start,end-start,DSK,'realcpc.dsk'
   endif
