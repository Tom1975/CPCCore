; P13DMA : does the INT of the DMA channel 2 interrupt the Z80 ? (README.md, P12 / P13)
; acid (Kevin Thacker), asic1.dsk DMATEST, "dma int request test (dcsr bits)" : with IVR bit 0 = 1, the routine of
; the interrupt reads the INT flag of channel 0 (#40) and 1 (#20) in DCSR, and expects #00 for channel 2. On the
; photo of PLUSDMA2, the INT of channel 2 (3 channels, IVR = 1) never draws its bar. Sugarbox : #10, at once.
; For each case : the channels of the case run INT + STOP (the others STOP), IM 2 : the vector the ASIC gives
; (IVR bits 7-3 = 0 : source in bits 2-1, raster 6, channel 0 4, channel 1 2, channel 2 0) selects a routine that
; notes the source and DCSR. PRI on line 199 (raster interrupts are ignored). Up to 400 lines.
; p13_results : per case, source (#FF : no DMA interrupt), DCSR in the routine, lines, DCSR at the end.
; Included by plus.asm (P13_TABLE in the RAM of the program) and plusc.asm (cartridge, CART).
p13_run:
   di
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
   ld hl,P13_TABLE      ; IM 2 table : 128 words, the routine that ignores the interrupt
   ld b,128
.fill:
   ld (hl),p13_other & #FF
   inc hl
   ld (hl),p13_other >> 8
   inc hl
   djnz .fill
   ld hl,p13_vectors    ; vectors 0, 2, 4, 6 : channel 2, 1, 0, raster
   ld de,P13_TABLE
   ld bc,8
   ldir
   ifdef CART
   ld hl,p13_lists_src  ; the DMA reads the RAM, not the cartridge
   ld de,P13_LISTS
   ld bc,4
   ldir
   endif
   ld a,P13_TABLE >> 8
   ld i,a
   im 2
   ld ix,p13_cases
   ld iy,p13_results
.case:
   ld a,(ix+0)
   or a
   jp z,.end
   xor a
   ld (#6C0F),a         ; DMA stopped
   ld a,#70
   ld (#6C0F),a         ; INT flags cleared
   xor a
   ld (#6C02),a         ; prescalers 0
   ld (#6C06),a
   ld (#6C0A),a
   ld a,(ix+1)
   ld (#6805),a         ; IVR
   ld a,199
   ld (#6800),a         ; PRI : line 199
   ld c,(ix+2)          ; SAR : INT + STOP for the channels of the INT mask, STOP for the others
   ld hl,P13_LIST_STOP
   bit 0,c
   jr z,.sar0
   ld hl,P13_LIST_INT
.sar0:
   ld (#6C00),hl
   ld hl,P13_LIST_STOP
   bit 1,c
   jr z,.sar1
   ld hl,P13_LIST_INT
.sar1:
   ld (#6C04),hl
   ld hl,P13_LIST_STOP
   bit 2,c
   jr z,.sar2
   ld hl,P13_LIST_INT
.sar2:
   ld (#6C08),hl
   ld a,#FF
   ld (p13_src),a
   ld (p13_dcsr),a
   WAIT_VSYNC_RISE
   ifdef CART
   ld bc,#7F9A          ; RMR bit 4 : R52 and the interrupt pending cleared (mode 2, the cartridge stays in)
   else
   ld bc,#7F9C          ; RMR bit 4 : R52 and the interrupt pending cleared (ROMs off, the program is at #8000)
   endif
   out (c),c
   ld a,(ix+0)
   ld (#6C0F),a         ; the channels start on the next HSYNC
   ld de,0
   ei
.line:                  ; 64 us
   ld a,(p13_src)       ; 4
   inc a                ; 1
   jr nz,.got           ; 2
   inc de               ; 2
   ld a,d               ; 1
   cp 400 / 256 + 1     ; 2  (512 lines : enough)
   jr z,.got            ; 2
   NOPS 47
   jr .line             ; 3
.got:
   di
   ld a,(#6C0F)
   ld (iy+3),a          ; DCSR at the end
   ld a,(p13_src)
   ld (iy+0),a
   ld a,(p13_dcsr)
   ld (iy+1),a
   ld (iy+2),e          ; lines (low byte)
   xor a
   ld (#6C0F),a
   ld a,#70
   ld (#6C0F),a
   ld de,4
   add iy,de
   ld de,3
   add ix,de
   jp .case
.end:
   xor a
   ld (#6800),a         ; PRI back to 0
   ld (#6805),a
   im 1
   ld bc,#7FA0          ; RMR2 : ASIC registers out
   out (c),c
   ret
; Routines of the IM 2 vectors : the source and DCSR, interrupts left off (an INT not cleared would come back)
p13_dma2:
   push af
   ld a,0
   jr p13_note
p13_dma1:
   push af
   ld a,2
   jr p13_note
p13_dma0:
   push af
   ld a,4
p13_note:
   ld (p13_src),a
   ld a,(#6C0F)
   ld (p13_dcsr),a
   pop af
   reti
p13_other:              ; raster (PRI) or anything else : ignored
   ei
   reti
p13_vectors: dw p13_dma2, p13_dma1, p13_dma0, p13_other

p13_lists_src: dw #4030, #4020   ; INT + STOP ; STOP

; DCSR enable, IVR, INT mask (channels that run INT + STOP)
p13_cases:
   db 1,0,1 : db 2,0,2 : db 4,0,4
   db 1,1,1 : db 2,1,2 : db 4,1,4
   db 7,1,7 : db 7,1,4
   db 0

p13_names:              ; 16 characters each
   db "C0 IVR=0        ",0
   db "C1 IVR=0        ",0
   db "C2 IVR=0        ",0
   db "C0 IVR=1        ",0
   db "C1 IVR=1        ",0
   db "C2 IVR=1        ",0
   db "C0+C1+C2 IVR=1  ",0
   db "3 CANAUX INT C2 ",0

p13_show:
   ld hl,txt_res_p13
   call print_str
   ld ix,p13_names
   ld iy,p13_results
   ld b,8
.case:
   push bc
   ld hl,txt_newline
   call print_str
   push ix
   pop hl
   call print_str
   ld de,17
   add ix,de
   ld a,(iy+0)          ; source
   cp #FF
   jr nz,.int
   ld hl,txt_p13_noint
   call print_str
   jr .end_dcsr
.int:
   call p13_space_hex
   ld a,(iy+1)          ; DCSR in the routine
   call p13_space_hex
   ld a,' '
   call print_char
   ld l,(iy+2)          ; lines
   ld h,0
   call print_dec
.end_dcsr:
   ld a,(iy+3)          ; DCSR at the end
   call p13_space_hex
   ld de,4
   add iy,de
   pop bc
   djnz .case
   ret
p13_space_hex:
   push af
   ld a,' '
   call print_char
   pop af
   jp print_hex

txt_res_p13:   db 13,10,15,2,"P13 : INT DMA EN IM 2 : SOURCE,",13,10,"DCSR (ROUTINE), LIGNES, DCSR (FIN)",13,10,15,1,0
txt_p13_noint: db " PAS D'INT",0

   ifdef CART
P13_TABLE      equ #8800
P13_LISTS      equ #8A00
p13_results    equ #8A10
p13_src        equ #8A40
p13_dcsr       equ #8A41
   else
   align 256
P13_TABLE:     ds 256
P13_LISTS:     dw #4030, #4020
p13_results:   ds 8 * 4
p13_src:       db 0
p13_dcsr:      db 0
   endif
P13_LIST_INT   equ P13_LISTS
P13_LIST_STOP  equ P13_LISTS + 2
