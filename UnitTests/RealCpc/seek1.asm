; FDC : how long does a SEEK of one track last, and what does the FDC do with a command sent while the interrupt of
; the end of a SEEK is still pending ? (disquettes_test SEEKJIT4 and the game Titan disagree, RealCpc README part 0)
; SPECIFY A1 03 (as AMSDOS and the CP/M boot : SRT = A), RECALIBRATE, then for each delay of sk_delays : SEEK from
; track 0 to track 1, wait the delay (from the last byte of the SEEK to the SENSE INTERRUPT command), SENSE INTERRUPT
; (ST0 kept), then 30 ms later a second SENSE INTERRUPT (ST0 kept), then back to track 0.
; ST0 #80 : invalid command, the SEEK was not finished ; #20 or #21 : end of the SEEK.
; Then : SEEK to track 1, 40 ms (the SEEK is finished, its interrupt not read), READ ID : its first result byte and
; the number of result bytes, then a SENSE INTERRUPT. Then : SEEK to track 1 and, 2 ms later (the head still stepping),
; READ DATA of sector C1 : first result byte, number of result bytes, then 30 ms later a SENSE INTERRUPT. Titan reads
; right after a SENSE INTERRUPT that gave #80 (Sugarbox : invalid command while stepping). Included by fdc.asm.

; 1000 us. Uses A, E.
sk_ms:
   ld e,a
.ms:
   DELAY_LINES 15       ; 961
   NOPS 32
   dec e                ; 1
   jr nz,.ms            ; 3 (+ CALL 5, LD 1, RET 3 once)
   ret

; Sends A as a command byte. Uses BC, E, HL.
sk_send:
   ld e,a
   jp fdc_send

; Result phase : E = first byte, D = number of bytes (0 : no result). Uses A, BC, HL.
sk_result:
   ld d,0
   ld e,#FF
.next:
   ld bc,#FB7E
   ld hl,0
.w:
   in a,(c)
   jp m,.rqm
   dec hl
   ld a,h
   or l
   jr nz,.w
   ret
.rqm:
   bit 6,a              ; DIO : 0 = back to the command phase
   ret z
   and #20              ; EXM : a data byte of the execution phase, not kept
   ld l,a
   inc c
   in a,(c)
   dec c
   inc l
   dec l
   jr nz,.next
   inc d
   dec d
   jr nz,.kept
   ld e,a
.kept:
   inc d
   NOPS 4
   jr .next

; SENSE INTERRUPT until the ST0 is not #80 (256 times at most). E = last ST0.
sk_sense_wait:
   ld b,0
.again:
   push bc
   ld a,#08
   call sk_send
   call sk_result
   pop bc
   ld a,e
   cp #80
   ret nz
   ld a,1
   push bc
   call sk_ms
   pop bc
   djnz .again
   ret

; Sends the B bytes at HL as a command. Stops after the first byte if the FDC is already in its result phase
; (invalid command). Uses A, BC, DE, HL.
sk_cmd:
   ld a,(hl)
   inc hl
   push bc
   push hl
   call sk_send
   ld bc,#FB7E          ; RQM again : DIO 1 = result phase already
   ld hl,0
.w:
   dec hl
   ld a,h
   or l
   jr z,.out
   in a,(c)
   jp p,.w
.out:
   pop hl
   pop bc
   and #40
   ret nz
   djnz sk_cmd
   ret

; SEEK drive A to track A
sk_seek:
   push af
   ld a,#0F
   call sk_send
   xor a
   call sk_send
   pop af
   jp sk_send

sk_run:
   di
   ld bc,#FA7E          ; motor on
   ld a,1
   out (c),a
   ld d,50              ; 1 s
.spin:
   WAIT_VSYNC_RISE
   dec d
   jr nz,.spin
   ld a,#03             ; SPECIFY : SRT A, HUT 1, HLT 1, no DMA
   call sk_send
   ld a,#A1
   call sk_send
   ld a,#03
   call sk_send
   ld a,#07             ; RECALIBRATE drive A
   call sk_send
   xor a
   call sk_send
   call sk_sense_wait
   ld ix,sk_delays
   ld iy,sk_st0
.delay:
   ld a,(ix+0)
   or a
   jr z,.readid
   ld a,1
   call sk_seek
   ld a,(ix+0)
   call sk_ms
   ld a,#08
   call sk_send
   call sk_result
   ld (iy+0),e
   ld a,30
   call sk_ms
   ld a,#08
   call sk_send
   call sk_result
   ld (iy+1),e
   call sk_sense_wait   ; nothing left pending
   xor a
   call sk_seek
   call sk_sense_wait
   inc ix
   inc iy
   inc iy
   jr .delay
.readid:
   ld a,1
   call sk_seek
   ld a,40
   call sk_ms
   ld hl,sk_cmd_rid
   ld b,2
   call sk_cmd
   call sk_result
   ld (sk_rid),de
   ld a,#08
   call sk_send
   call sk_result
   ld a,e
   ld (sk_rid_sense),a
   call sk_sense_wait
   xor a
   call sk_seek
   call sk_sense_wait
   ld a,1               ; READ DATA while the head steps
   call sk_seek
   ld a,2
   call sk_ms
   ld hl,sk_cmd_rd
   ld b,9
   call sk_cmd
   call sk_result
   ld (sk_rd),de
   ld a,30
   call sk_ms
   ld a,#08
   call sk_send
   call sk_result
   ld a,e
   ld (sk_rd_sense),a
   call sk_sense_wait
   xor a
   call sk_seek
   call sk_sense_wait
   ld bc,#FA7E          ; motor off
   xor a
   out (c),a
   ei
   ret

sk_show:
   ld hl,sk_txt
   call print_str
   ld ix,sk_delays
   ld iy,sk_st0
.row:
   ld a,(ix+0)
   or a
   jr z,.rid
   ld hl,txt_newline
   call print_str
   ld a,' '
   call #BB5A
   ld l,(ix+0)
   ld h,0
   call print_dec
   ld hl,sk_txt_ms
   call print_str
   ld a,(iy+0)
   call print_hex
   ld a,' '
   call #BB5A
   call #BB5A
   call #BB5A
   ld a,(iy+1)
   call print_hex
   inc ix
   inc iy
   inc iy
   jr .row
.rid:
   ld hl,sk_txt_rid
   call print_str
   ld a,(sk_rid)
   call print_hex
   ld hl,sk_txt_n
   call print_str
   ld a,(sk_rid+1)
   ld l,a
   ld h,0
   call print_dec
   ld hl,sk_txt_s
   call print_str
   ld a,(sk_rid_sense)
   call print_hex
   ld hl,sk_txt_rd
   call print_str
   ld a,(sk_rd)
   call print_hex
   ld hl,sk_txt_n
   call print_str
   ld a,(sk_rd+1)
   ld l,a
   ld h,0
   call print_dec
   ld hl,sk_txt_s
   call print_str
   ld a,(sk_rd_sense)
   call print_hex
   ld hl,sk_txt_end
   jp print_str

sk_txt:     db 15,2,"FDC : SEEK D'UNE PISTE (SRT A)",13,10,15,1
            db "ATTENTE  SENSE  SENSE 30 MS APRES",0
sk_txt_ms:  db " MS     ",0
sk_txt_rid: db 13,10,13,10,"READ ID, INTERRUPTION NON LUE :",13,10,"ST0 ",0
sk_txt_n:   db " (",0
sk_txt_s:   db " OCTETS), SENSE APRES : ",0
sk_txt_rd:  db 13,10,13,10,"READ DATA 2 MS APRES LE SEEK :",13,10,"ST0 ",0
sk_txt_end: db 13,10,13,10,"80 : PAS FINI / INVALIDE ; 20 : FINI",0

sk_delays:  db 2,4,6,7,8,9,10,11,12,13,14,16,20,0
sk_st0:     ds 26
sk_rid:     dw 0
sk_rid_sense: db 0
sk_rd:      dw 0
sk_rd_sense: db 0
sk_cmd_rid: db #4A,0                                ; READ ID, MFM, drive A head 0
sk_cmd_rd:  db #46,0,1,0,#C1,2,#C1,#2A,#FF          ; READ DATA, MFM, C1 of track 1
