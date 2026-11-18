; FDC : does RQM fall between two result bytes ? Neither the uPD765 datasheet nor 64nops gives a time. Sugarbox
; keeps RQM at 1 during the whole result phase (FDC.cpp).
; READ ID (MFM, drive A, head 0) on the disk in the drive, then for each of its 7 result bytes : the byte, and the
; main status register read 5, 9, 13, 17 and 21 us after it (bit 7 = RQM, bit 6 = DIO, bit 4 = busy). #D0 : the
; next byte is ready ; #50 : RQM down ; #80 after the last byte : back to the command phase.
; Results : fdc_smp (7 x 5 status bytes), fdc_ok (0 : no answer). Included by menu.asm.

macro FDC_RESULT_BYTE i
   ld bc,#FB7F
   in a,(c)             ; the result byte (not kept), I/O on the 3rd us
   dec c                ; 1 : main status register
   in a,(c)             ; 5 us after
   in d,(c)             ; 9
   in e,(c)             ; 13
   in h,(c)             ; 17
   in l,(c)             ; 21
   ld (fdc_smp+5*{i}),a
   ld a,d
   ld (fdc_smp+5*{i}+1),a
   ld a,e
   ld (fdc_smp+5*{i}+2),a
   ld a,h
   ld (fdc_smp+5*{i}+3),a
   ld a,l
   ld (fdc_smp+5*{i}+4),a
mend

fdc_run:
   di
   xor a
   ld (fdc_ok),a
   ld bc,#FA7E          ; motor on
   ld a,1
   out (c),a
   ld e,50              ; 1 s
.spin:
   WAIT_VSYNC_RISE
   dec e
   jr nz,.spin
   ld e,#4A             ; READ ID, MFM
   call fdc_send
   jp nc,fdc_off
   ld e,0               ; drive A, head 0
   call fdc_send
   jp nc,fdc_off
   ld hl,0              ; result phase : RQM and DIO, about 1 s at most
.exec:
   dec hl
   ld a,h
   or l
   jp z,fdc_off
   ld bc,#FB7E
   in a,(c)
   and #C0
   cp #C0
   jr nz,.exec
   ld a,1
   ld (fdc_ok),a
   FDC_RESULT_BYTE 0
   call fdc_wait_rqm
   FDC_RESULT_BYTE 1
   call fdc_wait_rqm
   FDC_RESULT_BYTE 2
   call fdc_wait_rqm
   FDC_RESULT_BYTE 3
   call fdc_wait_rqm
   FDC_RESULT_BYTE 4
   call fdc_wait_rqm
   FDC_RESULT_BYTE 5
   call fdc_wait_rqm
   FDC_RESULT_BYTE 6
fdc_off:
   ld bc,#FA7E          ; motor off
   xor a
   out (c),a
   ret

; Waits for RQM (bit 7) with a time limit
fdc_wait_rqm:
   ld bc,#FB7E
   ld hl,0
.w:
   in a,(c)
   ret m
   dec hl
   ld a,h
   or l
   jr nz,.w
   ret

; Sends E when the FDC asks for a command byte (RQM=1, DIO=0). Carry set if sent, clear after the time limit.
fdc_send:
   ld bc,#FB7E
   ld hl,0
.w:
   dec hl
   ld a,h
   or l
   ret z                ; carry clear
   in a,(c)
   and #C0
   cp #80
   jr nz,.w
   inc c
   out (c),e
   scf
   ret

; Result page
fdc_show:
   ld hl,fdc_txt
   call print_str
   ld a,(fdc_ok)
   or a
   jr nz,.lines
   ld hl,fdc_txt_none
   jp print_str
.lines:
   ld hl,fdc_smp
   ld b,7
   ld c,1
.line:
   push bc
   push hl
   ld hl,fdc_txt_byte
   call print_str
   ld a,c
   add a,'0'
   call #BB5A
   ld a,':'
   call #BB5A
   pop hl
   ld b,5
.smp:
   ld a,' '
   call #BB5A
   ld a,(hl)
   call print_hex
   inc hl
   djnz .smp
   pop bc
   inc c
   djnz .line
   ld hl,fdc_txt_end
   jp print_str

fdc_txt:      db 15,2,"FDC : RQM ENTRE LES OCTETS DE RESULTAT",13,10,15,1
              db "READ ID ; STATUT 5, 9, 13, 17, 21 US",13,10,"APRES CHAQUE OCTET :",13,10,0
fdc_txt_byte: db 13,10,"OCTET ",0
fdc_txt_end:  db 13,10,13,10,"D0 : OCTET SUIVANT PRET",13,10,"50 : RQM RETOMBE",13,10,"80 : FIN DU RESULTAT",0
fdc_txt_none: db 13,10,"PAS DE REPONSE : DISQUETTE DANS A ?",0

fdc_ok:  db 0
fdc_smp: ds 35
