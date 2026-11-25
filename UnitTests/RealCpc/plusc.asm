; PLUSC : the 6128 Plus tests of README.md as a cartridge (plusc.cpr), for a Plus without disk drive or with its
; firmware out of the way. No firmware : the cartridge starts at #0000 (page 0 of the cartridge as lower ROM) and
; sets the Gate Array, the PPI, the CRTC and the inks itself, then prints with its own 8x8 font (font.asm, mode 2).
; It runs P12 (p12dma.asm) and P13 (p13dma.asm) once and shows their pages ; the reset button runs them again.
; Built by build.sh : rasm plusc.asm -o plusc (RASM writes plusc.cpr).
CART equ 1
   include 'common.asm'
   buildcpr
   bank 0
   org #0000
   di
   jp cart_start

   org #0038            ; interrupts are off ; just in case
   ei
   ret

cart_start:
   ld sp,#BFF0
   im 1
   ld bc,#7F8A          ; Gate Array : mode 2, lower ROM (the cartridge) on, upper ROM off
   out (c),c
   ld bc,#F782          ; PPI : port A output, port B input (VSYNC), port C output
   out (c),c
   ld bc,#F600
   out (c),c
   ld hl,cart_crtc      ; CRTC : the values of the firmware, screen at #C000
   ld e,0
.crtc:
   ld b,#BC
   out (c),e
   ld b,#BD
   ld a,(hl)
   out (c),a
   inc hl
   inc e
   ld a,e
   cp 14
   jr nz,.crtc
   ld bc,#7F00          ; pen 0 : black
   out (c),c
   ld a,#54
   out (c),a
   ld bc,#7F01          ; pen 1 : bright white
   out (c),c
   ld a,#4B
   out (c),a
   ld bc,#7F10          ; border : black
   out (c),c
   ld a,#54
   out (c),a
   ld hl,#C000          ; screen cleared
   ld de,#C001
   ld bc,#3FFF
   ld (hl),0
   ldir
   xor a
   ld (cur_col),a
   ld (cur_row),a
   ld hl,txt_cart
   call print_str
   call p12_run
   call p12_show
   call p13_run
   call p13_show
   ld hl,txt_cart_end
   call print_str
.stop:
   di
   halt
   jr .stop

cart_crtc: db 63,40,46,#8E,38,0,25,30,0,7,0,0,#30,0

   include 'p12dma.asm'
   include 'p13dma.asm'

; ---------------------------------------------------------------------------------------------------------------
; Text without firmware : mode 2, 80 columns, 25 rows. Codes : 13 column 0, 10 next row, 15,n pen (ignored).
print_str:              ; text HL ending with 0
   ld a,(hl)
   or a
   ret z
   inc hl
   cp 15
   jr nz,.char
   inc hl               ; 15,n : the pen is not used in mode 2
   jr print_str
.char:
   call print_char
   jr print_str

print_char:             ; A, keeps every register
   push af
   push bc
   push de
   push hl
   cp 13
   jr nz,.lf
   xor a
   ld (cur_col),a
   jr .done
.lf:
   cp 10
   jr nz,.glyph
   ld hl,cur_row
   inc (hl)
   jr .done
.glyph:
   cp 32
   jr c,.space
   cp 91
   jr c,.known
.space:
   ld a,32
.known:
   sub 32               ; glyph : font + 8 * (A - 32)
   ld l,a
   ld h,0
   add hl,hl
   add hl,hl
   add hl,hl
   ld de,font
   add hl,de
   ex de,hl             ; DE : glyph
   ld a,(cur_row)       ; screen : #C000 + 80 * row + column
   ld l,a
   ld h,0
   add hl,hl
   add hl,hl
   add hl,hl
   add hl,hl            ; 16 * row
   ld b,h
   ld c,l
   add hl,hl
   add hl,hl            ; 64 * row
   add hl,bc            ; 80 * row
   ld a,(cur_col)
   ld c,a
   ld b,#C0
   add hl,bc
   ld b,8
.row:
   ld a,(de)
   ld (hl),a
   inc de
   ld a,h
   add a,8              ; next pixel line : + #800
   ld h,a
   djnz .row
   ld hl,cur_col
   inc (hl)
   ld a,(hl)
   cp 80
   jr c,.done
   ld (hl),0
   ld hl,cur_row
   inc (hl)
.done:
   pop hl
   pop de
   pop bc
   pop af
   ret

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
   jp print_char

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
   jp print_char

   include 'font.asm'

txt_newline:  db 13,10,0
txt_cart:     db "TESTS DU 6128 PLUS SUR VRAI CPC (CARTOUCHE PLUSC.CPR, SANS FIRMWARE)",13,10,0
txt_cart_end: db 13,10,"TERMINE. PHOTOGRAPHIER L'ECRAN. RESET : RECOMMENCER.",0

cart_end:
   assert cart_end <= #4000

cur_col equ #8600
cur_row equ #8601
