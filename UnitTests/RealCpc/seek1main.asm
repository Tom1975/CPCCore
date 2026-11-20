; SEEK1 : RUN"SEEK1" from realcpc.dsk (built by build.sh after the menu ; the menu has no room left). See seek1.asm.
; Shows its result page, then waits for a key and goes back to BASIC.
   include 'common.asm'
   org #4000
   run start
start:
   ld a,1
   call #BC0E           ; SCR SET MODE : mode 1, screen cleared
   ld hl,txt_wait
   call print_str
   call sk_run
   ld a,1
   call #BC0E
   call sk_show
   ld hl,txt_key
   call print_str
   call #BB18           ; KM WAIT KEY
   rst 0

print_str:                  ; text HL ending with 0
   ld a,(hl)
   or a
   ret z
   call #BB5A
   inc hl
   jr print_str

print_dec:              ; HL in decimal, no leading zeros
   ld e,0
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

; Sends E when the FDC asks for a command byte (RQM=1, DIO=0). Carry set if sent, clear after the time limit.
fdc_send:
   ld bc,#FB7E
   ld hl,0
.w:
   dec hl
   ld a,h
   or l
   ret z
   in a,(c)
   and #C0
   cp #80
   jr nz,.w
   inc c
   out (c),e
   scf
   ret

   include 'seek1.asm'

txt_wait:    db "SEEK1 : DISQUETTE DANS A. ENVIRON 5 S...",0
txt_newline: db 13,10,0
txt_key:     db 13,10,13,10,15,2,"PHOTO, PUIS UNE TOUCHE.",0

end:
   save 'SEEK1.BIN',start,end-start,DSK,'realcpc.dsk'
