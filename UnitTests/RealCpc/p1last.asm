; P1 (README, part 1 bis) : when does the CRTC 1 sample the last line state (C4=R4, C9=R9) ?
; Sugarbox : on the rising edge of the output of the C0=R0 comparator (CRTC::SampleLastLine1) ; with R0 written
; equal to the C0 in progress, that is the instant of the write. Firmware values : R4=38, R9=7, R5=0.
; (a) On the last line of the frame (C4=38, C9=7), R0=20 written on C0=20, then R0=63 again at once. The frame
;     ends : about 1535 steps of 10 us to the next VSYNC (Sugarbox 1535). C4 overflows : about 6100 steps.
; (b) On the last line, R4=39 written with its I/O on C0=60, 61, 62, 63 (=R0), then C0=0 of the next line ; R4=38
;     again after the next VSYNC. Frame ended : about 1536 steps ; one more row (C4=39) : about 1587. Sugarbox :
;     1587 on C0=60 to 62, 1536 on C0=63 and 0 (the state is taken at the start of the character C0=R0).
;     The first position where the frame ends says when the state is sampled.
; Results : p1_a, p1_b (5 words). Included by crtc.asm.

P1A_LINES equ 8         ; set with the emulator : I/O of R0=20 on C4=38, C9=7, C0=20
P1A_NOPS  equ 63
P1B_LINES equ 9         ; I/O of R4=39 on C4=38, C9=7, C0=60 for the first position
P1B_NOPS  equ 39

macro P1B_POSITION k
   call sync_frame
   ld bc,#BC04          ; R4 selected
   out (c),c
   DELAY_LINES P1B_LINES
   NOPS P1B_NOPS+{k}
   ld bc,#BD27          ; R4 = 39
   out (c),c
   call count_to_vsync_rise
   ld (p1_b+2*{k}),de
   CRTC_WRITE 4,38      ; after the VSYNC : C4 = 30
mend

p1_run:
   call int_patch
   call sync_frame
   ld bc,#BC00          ; R0 selected
   out (c),c
   DELAY_LINES P1A_LINES
   NOPS P1A_NOPS
   ld bc,#BD14          ; R0 = 20
   out (c),c
   ld bc,#BD3F          ; R0 = 63, 7 us later
   out (c),c
   call count_to_vsync_rise
   ld (p1_a),de
   P1B_POSITION 0
   P1B_POSITION 1
   P1B_POSITION 2
   P1B_POSITION 3
   P1B_POSITION 4
   jp int_restore

; Result page
p1_show:
   ld hl,p1_txt_a
   call print_str
   ld hl,(p1_a)
   call print_dec
   ld hl,p1_txt_b
   call print_str
   ld hl,p1_b
   ld (p1_ptr),hl
   ld b,5
   ld c,60
.pos:
   push bc
   ld hl,p1_txt_pos
   call print_str
   ld a,c
   cp 64
   jr nz,.c0
   xor a
.c0:
   ld l,a
   ld h,0
   call print_dec
   ld hl,p1_txt_colon
   call print_str
   ld hl,(p1_ptr)
   ld e,(hl)
   inc hl
   ld d,(hl)
   inc hl
   ld (p1_ptr),hl
   ex de,hl
   call print_dec
   pop bc
   inc c
   djnz .pos
   ret

p1_txt_a:   db 15,2,"P1 : DERNIERE LIGNE DU CRTC 1",13,10,13,10,15,1
            db "(A) R0=C0=20 : ",0
p1_txt_b:   db 13,10,13,10,"(B) R4=39 ECRIT SUR C0 =",0
p1_txt_pos: db 13,10,"   ",0
p1_txt_colon: db " : ",0

p1_a: dw 0
p1_b: dw 0,0,0,0,0
p1_ptr: dw 0
