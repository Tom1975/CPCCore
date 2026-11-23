; P8 ter (key 5, README part 0) : where does the us more of the VSYNC with R0 = 0 come from ? Real CRTC 1 (P8 bis
; (F), 2026-11-21) : every value is the Sugarbox one for 1 us more of R0 = 0 ; the VSYNC lasts 33 us with R0 = 0 there,
; 32 in Sugarbox (C3h counts half lines, CRTC_1.cpp). In (F) the VSYNC starts during the R0 = 0 : here R0 = 0 is
; written on C0 = 0 of a line inside a VSYNC already running (firmware : R7 = 30, VSYNC from the line 240), as
; P8D_RUN / p8f_pre do it but 5 lines later (C4 = 30, C9 = 4 : 4 lines of VSYNC done, 24 half lines left) and 9 lines
; later (C4 = 31, C9 = 0 : 8 lines done, 16 half lines left). R0 = 63 n us after, steps of 10 us to the end of the
; VSYNC. Same shift as (F) : the us is lost in the count during R0 = 0 ; the Sugarbox values : it comes from the VSYNC
; start with R0 = 0.
; (G) 4 lines done, R0 = 0 for 16 ... 31 us ; (H) 8 lines done, R0 = 0 for 8 ... 23 us. Results : p8t_g, p8t_h
; (16 words each). Included by crtc.asm, uses p8b_target / p8b_store / p8_target / p8_val of p8vsync.asm.

; DELAY_LINES n lasts 64 n + 1 us : two of them 1 us more than one, taken off the NOPS
macro P8T_PRE extra
   call sync_frame
   ld bc,#BC00          ; R0 selected
   out (c),c
   DELAY_LINES P8A_LINES
   DELAY_LINES {extra}
   NOPS P8A_NOPS-6
   ld hl,(p8_target)    ; 5
   ld bc,#BD00          ; R0 = 0
   out (c),c
   jp (hl)              ; 1
mend

p8t_run:
   call int_patch
   ld hl,p8t_targets_g
   ld de,p8t_g
   ld b,16
.g:
   call p8b_target
   call p8tg_pre
   call p8b_store
   djnz .g
   ld hl,p8t_targets_h
   ld de,p8t_h
   ld b,16
.h:
   call p8b_target
   call p8th_pre
   call p8b_store
   djnz .h
   jp int_restore

; (G) : 5 lines after the line of P8D_RUN
p8tg_pre:
   P8T_PRE 5
   ds 40,0              ; NOP
p8tg_sled_end:
   ld bc,#BD3F          ; R0 = 63, n us after the first I/O
   out (c),c
   call count_to_vsync_fall
   ld (p8_val),de
   ret

; (H) : 9 lines after the line of P8D_RUN
p8th_pre:
   P8T_PRE 9
   ds 40,0              ; NOP
p8th_sled_end:
   ld bc,#BD3F          ; R0 = 63, n us after the first I/O
   out (c),c
   call count_to_vsync_fall
   ld (p8_val),de
   ret

; n us between the two I/O : n - 8 NOP of the sled (as p8f_targets)
p8t_targets_g:
   repeat 16,k
   dw p8tg_sled_end-(15+k-8)
   rend
p8t_targets_h:
   repeat 16,k
   dw p8th_sled_end-(7+k-8)
   rend

; Result page : 4 rows of 8 numbers
p8t_show:
   ld hl,p8t_txt_g
   call print_str
   ld hl,p8t_g
   call p8t_row
   ld hl,p8t_txt_g2
   call print_str
   ld hl,p8t_g+16
   call p8t_row
   ld hl,p8t_txt_h
   call print_str
   ld hl,p8t_h
   call p8t_row
   ld hl,p8t_txt_h2
   call print_str
   ld hl,p8t_h+16
   jp p8t_row

; 8 words from HL on one row of 39 characters at most : no indent, a space between two numbers only
p8t_row:
   ld b,8
.n:
   push bc
   ld e,(hl)
   inc hl
   ld d,(hl)
   inc hl
   push hl
   ex de,hl
   call print_dec
   pop hl
   pop bc
   ld a,b
   cp 1
   jr z,.last
   push hl
   push bc
   ld a,' '
   call #BB5A
   pop bc
   pop hl
.last:
   djnz .n
   ret

p8t_txt_g:  db 15,2,"P8 TER : R0=0 PENDANT LA VSYNC",13,10,13,10,15,1
            db "(G) 4 LIGNES, R0=0 16 17 ... 23 US :",13,10,0
p8t_txt_g2: db 13,10,"    R0=0 24 25 ... 31 US :",13,10,0
p8t_txt_h:  db 13,10,13,10,"(H) 8 LIGNES, R0=0 8 9 ... 15 US :",13,10,0
p8t_txt_h2: db 13,10,"    R0=0 16 17 ... 23 US :",13,10,0

p8t_g:    ds 32
p8t_h:    ds 32
