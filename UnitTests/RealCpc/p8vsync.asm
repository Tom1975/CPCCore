; P8 (README, part 1 bis) : what clocks the VSYNC line counter C3h of the CRTC 1 ?
; Sugarbox : the end of the HSYNC of the CRTC (CRTC_1.cpp) ; without HSYNC, C3h is frozen. Firmware values :
; R0=63, R2=46, R7=30, R9=7, VSYNC on C4=30 (line 240), 16 lines.
; (a) R0=0 written on C0=0 of the line C4=29, C9=7, R0=63 40 us later. Steps of 10 us from the R0=63 write to the
;     end of the VSYNC ; reference : the same with R0=63 written instead of R0=0. With R0=0 the lines last 1 us and
;     C4 counts on (13.2.6) : the VSYNC starts at once. C3h frozen without HSYNC (Sugarbox) : about 102 (reference
;     105). C3h counting the lines of 1 us : the VSYNC ends during the 40 us, the count runs to the end of the
;     VSYNC of the next frame : about 2000.
; (b) R0=20, R2=40 (C0 never reaches R2 : no HSYNC) from the line C4=25 ; 3 ms after the VSYNC rose, is it still
;     on ? Then R0=63, R2=46 and steps of 10 us to its end. C3h frozen (Sugarbox) : on, then 100.
; (c) R7=20 written on the line C4=20, C9=0 (the VSYNC starts at once), its I/O on C0=#10 (before the HSYNC) then
;     on C0=#3C (after its end) : steps of 10 us from the write to the end of the VSYNC. Compendium 16.4.2 : 16 lines
;     minus C0+1 in both cases (about 100, then 96) ; Sugarbox : 101, then 103 (one line more after the HSYNC).
;(d) As (a), R0=0 during 10, 14, 18 ... 38 us : about 100 while the VSYNC goes on after it, about 1850 once it
;     ended during R0=0. SHAKER #008D (18 us : the VSYNC goes on) and (a) (40 us : ended) : the first 1850 tells
;     how fast C3h counts the lines of 1 us (one line each us : from about 17 us ; one of two : about 33 us).
; Results : p8_ref, p8_r0, p8_still, p8_rest, p8_c10, p8_c3c, p8_d (8 words). Included by menu.asm.

P8A_LINES equ 248         ; set with the emulator : I/O of R0 on C4=29, C9=7, C0=0
P8A_NOPS  equ 43
P8C_LINES equ 169         ; I/O of R7=20 on C4=20, C9=0, C0=#10
P8C_NOPS  equ 59

macro P8A_RUN value,result
   call sync_frame
   ld bc,#BC00          ; R0 selected
   out (c),c
   DELAY_LINES P8A_LINES
   NOPS P8A_NOPS
   ld bc,#BD00+{value}  ; R0 = value
   out (c),c
   NOPS 33
   ld bc,#BD3F          ; R0 = 63, 40 us after the first I/O
   out (c),c
   call count_to_vsync_fall
   ld ({result}),de
mend

macro P8D_RUN us,result
   call sync_frame
   ld bc,#BC00          ; R0 selected
   out (c),c
   DELAY_LINES P8A_LINES
   NOPS P8A_NOPS
   ld bc,#BD00          ; R0 = 0
   out (c),c
   NOPS {us}-7
   ld bc,#BD3F          ; R0 = 63, us after the first I/O
   out (c),c
   call count_to_vsync_fall
   ld ({result}),de
mend

macro P8C_RUN c0,result
   call sync_frame
   ld bc,#BC07          ; R7 selected
   out (c),c
   DELAY_LINES P8C_LINES
   NOPS P8C_NOPS+{c0}-#10
   ld bc,#BD14          ; R7 = 20
   out (c),c
   call count_to_vsync_fall
   ld ({result}),de
   CRTC_WRITE 7,30
mend

p8_run:
   call int_patch
   P8A_RUN #3F,p8_ref
   P8A_RUN 0,p8_r0
   ; (b)
   call sync_frame      ; line 302 : the line C4=25 (200) of the next frame is 210 lines later
   DELAY_LINES 210
   CRTC_WRITE 2,40
   CRTC_WRITE 0,20
   ld b,#F5
.rise:
   in a,(c)
   rra
   jr nc,.rise
   DELAY_LINES 46       ; 2945 us
   NOPS 55              ; 3000 us
   ld b,#F5
   in a,(c)
   and 1
   ld (p8_still),a
   CRTC_WRITE 0,63
   CRTC_WRITE 2,46
   call count_to_vsync_fall
   ld (p8_rest),de
   ; (c)
   P8C_RUN #10,p8_c10
   P8C_RUN #3C,p8_c3c
   ; (d)
   P8D_RUN 10,p8_d
   P8D_RUN 14,p8_d+2
   P8D_RUN 18,p8_d+4
   P8D_RUN 22,p8_d+6
   P8D_RUN 26,p8_d+8
   P8D_RUN 30,p8_d+10
   P8D_RUN 34,p8_d+12
   P8D_RUN 38,p8_d+14
   jp int_restore

; Result page
p8_show:
   ld hl,p8_txt_a
   call print_str
   ld hl,(p8_ref)
   call print_dec
   ld hl,p8_txt_a2
   call print_str
   ld hl,(p8_r0)
   call print_dec
   ld hl,p8_txt_b
   call print_str
   ld a,(p8_still)
   or a
   ld hl,p8_txt_yes
   jr nz,.yes
   ld hl,p8_txt_no
.yes:
   call print_str
   ld hl,p8_txt_b2
   call print_str
   ld hl,(p8_rest)
   call print_dec
   ld hl,p8_txt_c
   call print_str
   ld hl,(p8_c10)
   call print_dec
   ld hl,p8_txt_c2
   call print_str
   ld hl,(p8_c3c)
   call print_dec
   ld hl,p8_txt_d
   call print_str
   ld hl,p8_d
   ld b,8
.d:
   push bc
   ld e,(hl)
   inc hl
   ld d,(hl)
   inc hl
   push hl
   ex de,hl
   call print_dec
   ld a,' '
   call #BB5A
   pop hl
   pop bc
   ld a,b
   cp 5
   jr nz,.next
   push hl
   ld hl,p8_txt_d2
   call print_str
   pop hl
.next:
   djnz .d
   ret

p8_txt_a:  db 15,2,"P8 : COMPTEUR DE LIGNES VSYNC CRTC 1",13,10,13,10,15,1
           db "(A) REFERENCE : ",0
p8_txt_a2: db 13,10,"    R0=0 40 US : ",0
p8_txt_b:  db 13,10,13,10,"(B) VSYNC ACTIVE APRES 3 MS : ",0
p8_txt_yes: db "OUI",0
p8_txt_no:  db "NON",0
p8_txt_b2: db 13,10,"    PUIS : ",0
p8_txt_c:  db 13,10,13,10,"(C) R7 SUR C0=#10 : ",0
p8_txt_c2: db 13,10,"    R7 SUR C0=#3C : ",0
p8_txt_d:  db 13,10,13,10,"(D) R0=0 10 14 18 22 / 26 30 34 38 US :",13,10,"    ",0
p8_txt_d2: db 13,10,"    ",0

p8_ref:   dw 0
p8_r0:    dw 0
p8_still: db 0
p8_rest:  dw 0
p8_c10:   dw 0
p8_c3c:   dw 0
p8_d:     ds 16
