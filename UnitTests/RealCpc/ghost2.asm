; GHOST : CRTC 2, VSYNC FANTOME (compendium 15.4.4, 16.4.3). R0 = 63, R7 = 30, R2 = 50 and R3l = 11 to 15 (R3h = 0) :
; does the VSYNC pin rise ? About 90 ms of PPI port B reads each, interrupts off (no hang when it never rises).
; The compendium (schema p156) : R3l = 12 and 13 VSYNC, R3l = 14 and 15 none (the HSYNC reaches the C0 = 63 before
; C0 = 0 of the C4 = R7 line). VSINT (disquettes_test, real CRTC 2 with a M4 card) hangs and the picture rolls with
; R2 = 50, R3 = #0D : no VSYNC with R3l = 13 ? Results : gh_seen (5 bytes, 1 = VSYNC seen). Included by crtc.asm.

; A frame at most (25000 us) : the VSYNC may be missing
macro WAIT_VSYNC_RISE_TIMEOUT
   ld b,#F5
   ld hl,2500
@w:
   in a,(c)
   rra
   jr c,@out
   dec hl
   ld a,h
   or l
   jr nz,@w
@out:
mend

gh_run:
   di
   ld ix,gh_seen
   ld e,11
.r3:
   push de
   ld bc,#BC02
   out (c),c
   ld bc,#BD00+50
   out (c),c
   ld bc,#BC03
   out (c),c
   inc b                ; #BD
   out (c),e
   ld d,4               ; 2 frames to settle (a ghost VSYNC lasts 16 lines)
.settle:
   WAIT_VSYNC_RISE_TIMEOUT
   dec d
   jr nz,.settle
   ld b,#F5
   ld h,0
   ld de,6000           ; 6000 x 15 us
.poll:
   in a,(c)             ; 4
   and 1                ; 2
   or h                 ; 1
   ld h,a               ; 1
   dec de               ; 2
   ld a,d               ; 1
   or e                 ; 1
   jr nz,.poll          ; 3
   ld (ix+0),h
   inc ix
   pop de
   inc e
   ld a,e
   cp 16
   jr nz,.r3
   CRTC_WRITE 2,46
   CRTC_WRITE 3,#8E
   ei
   ret


gh_show:
   ld hl,gh_txt
   call print_str
   ld ix,gh_seen
   ld b,11
.row:
   push bc
   ld hl,gh_txt_r3
   call print_str
   pop bc
   push bc
   ld l,b
   ld h,0
   call print_dec
   ld hl,gh_txt_yes
   ld a,(ix+0)
   or a
   jr nz,.print
   ld hl,gh_txt_no
.print:
   call print_str
   inc ix
   pop bc
   inc b
   ld a,b
   cp 16
   jr nz,.row
   ld hl,gh_txt_end
   jp print_str

gh_txt:     db 15,2,"CRTC 2 : VSYNC FANTOME (R0=63, R2=50)",13,10,15,1,0
gh_txt_r3:  db 13,10,"R3L = ",0
gh_txt_yes: db " : VSYNC",0
gh_txt_no:  db " : PAS DE VSYNC",0
gh_txt_end: db 13,10,13,10,"COMPENDIUM : 11 12 13 VSYNC, 14 15 NON",0

gh_seen: ds 5
