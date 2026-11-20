; P12DMA : how long does a PAUSE of the ASIC DMA last when several channels pause together ? (README.md, P12)
; PLUSDMA measures the PAUSE with one channel only ; on the photo of PLUSDMA2 the INT of channel 2, after a
; PAUSE 30 of the three channels, never shows. For each case : PAUSE 30 (prescaler 0 or 1) then INT on the
; channels of the case, DCSR read once per line (64 us) for 400 lines from the line of the enable.
; p12_results : per case, the first line where the INT flag of channel 0, 1, 2 is set (#FFFF : never).
; Included by plus.asm (org #8000 : the ASIC registers are paged in #4000-#7FFF during the test) and by plusc.asm
; (cartridge, CART defined : no firmware, the DMA lists and the results go to the RAM at #8000).
p12_run:
   di
   ifdef CART
   ld hl,p12_lists_src  ; the DMA reads the RAM, not the cartridge
   ld de,P12_RAM
   ld bc,p12_lists_end-p12_lists_src
   ldir
   endif
   ld bc,#BC00          ; ASIC unlock sequence on the CRTC select port
   ld hl,p12_unlock
   ld d,17
.unlock:
   ld a,(hl)
   out (c),a
   inc hl
   dec d
   jr nz,.unlock
   ld bc,#7FB8          ; RMR2 : ASIC registers in #4000-#7FFF
   out (c),c
   ld ix,p12_cases
   ld iy,p12_results
.case:
   ld a,(ix+0)
   or a
   jp z,.end
   xor a
   ld (#6C0F),a         ; DMA stopped
   ld a,#70
   ld (#6C0F),a         ; INT flags cleared
   ld a,(ix+1)          ; prescaler of the three channels
   ld (#6C02),a
   ld (#6C06),a
   ld (#6C0A),a
   ld l,(ix+2)
   ld h,(ix+3)
   ld (#6C00),hl
   ld l,(ix+4)
   ld h,(ix+5)
   ld (#6C04),hl
   ld l,(ix+6)
   ld h,(ix+7)
   ld (#6C08),hl
   WAIT_VSYNC_RISE
   ld a,(ix+0)
   ld (#6C0F),a         ; the channels of the case start on the next HSYNC
   ld hl,p12_buf
   ld de,400
.line:                  ; 64 us
   ld a,(#6C0F)         ; 4
   ld (hl),a            ; 2
   inc hl               ; 2
   dec de               ; 2
   ld a,d               ; 1
   or e                 ; 1
   jr z,.done           ; 2
   NOPS 47
   jr .line             ; 3
.done:
   xor a
   ld (#6C0F),a
   ld a,#70
   ld (#6C0F),a
   ld c,#40             ; INT flag of channel 0, then 1, then 2
   ld b,3
.channel:
   push bc
   call p12_find
   pop bc
   ld (iy+0),l
   ld (iy+1),h
   inc iy
   inc iy
   srl c
   djnz .channel
   ld de,8
   add ix,de
   jp .case
.end:
   xor a                ; prescalers back to 0
   ld (#6C02),a
   ld (#6C06),a
   ld (#6C0A),a
   ld bc,#7FA0          ; RMR2 : ASIC registers out
   out (c),c
   ret

; C = flag : HL = first line of p12_buf with it, #FFFF if none
p12_find:
   ld hl,p12_buf
   ld de,0
.next:
   ld a,(hl)
   and c
   jr nz,.found
   inc hl
   inc de
   ld a,e
   cp #90               ; 400 = #0190
   jr nz,.next
   ld a,d
   cp #01
   jr nz,.next
   ld hl,#FFFF
   ret
.found:
   ex de,hl
   ret

p12_show:
   ld hl,txt_res_p12
   call print_str
   ld ix,p12_names
   ld iy,p12_results
   ld b,7
.case:
   push bc
   ld hl,txt_newline
   call print_str
   push ix
   pop hl
   call print_str       ; name of the case, 16 characters
   ld de,17
   add ix,de
   ld b,3
.value:
   push bc
   ld a,' '
   call print_char
   ld l,(iy+0)
   ld h,(iy+1)
   inc iy
   inc iy
   ld a,h
   and l
   inc a
   jr nz,.number
   ld hl,txt_p12_none
   call print_str
   jr .next
.number:
   call print_dec
.next:
   pop bc
   djnz .value
   pop bc
   djnz .case
   ret

; Lists : PAUSE 30, INT + STOP ; PAUSE 30, LOAD R7 #3F, STOP (PLUSDMA2) ; PAUSE 30, LOAD R7 #3F, INT + STOP
   align 2              ; the DMA ignores bit 0 of the address : the lists on an even address
p12_lists_src:
p12_list_int_s:  dw #101E, #4030
p12_list_load_s: dw #101E, #073F, #4020
p12_list_both_s: dw #101E, #073F, #4030
p12_lists_end:
   ifdef CART
P12_RAM       equ #8000
p12_list_int  equ P12_RAM + p12_list_int_s - p12_lists_src
p12_list_load equ P12_RAM + p12_list_load_s - p12_lists_src
p12_list_both equ P12_RAM + p12_list_both_s - p12_lists_src
p12_results   equ #8100
p12_buf       equ #8200
   else
p12_list_int  equ p12_list_int_s
p12_list_load equ p12_list_load_s
p12_list_both equ p12_list_both_s
   endif

; DCSR enable, prescaler, SAR 0, SAR 1, SAR 2
p12_cases:
   db 1,0 : dw p12_list_int, p12_list_int, p12_list_int
   db 3,0 : dw p12_list_int, p12_list_int, p12_list_int
   db 7,0 : dw p12_list_int, p12_list_int, p12_list_int
   db 4,0 : dw p12_list_int, p12_list_int, p12_list_int
   db 7,0 : dw p12_list_load, p12_list_load, p12_list_int
   db 1,0 : dw p12_list_both, p12_list_int, p12_list_int
   db 7,1 : dw p12_list_int, p12_list_int, p12_list_int
   db 0

p12_names:              ; 16 characters each
   db "C0 SEUL         ",0
   db "C0+C1           ",0
   db "C0+C1+C2        ",0
   db "C2 SEUL         ",0
   db "COMME PLUSDMA2  ",0
   db "C0 LOAD PUIS INT",0
   db "C0+C1+C2 PRESC 1",0

p12_unlock: db #FF,#00,#FF,#77,#B3,#51,#A8,#D4,#62,#39,#9C,#46,#2B,#15,#8A,#CD,#EE

txt_res_p12:  db 15,2,"P12 : PAUSE 30 PUIS INT, LIGNE DU",13,10,"DRAPEAU INT DE C0, C1, C2",13,10,15,1,0
txt_p12_none: db "--",0

   ifndef CART
p12_results: ds 7 * 6
p12_buf:     ds 400
   endif
