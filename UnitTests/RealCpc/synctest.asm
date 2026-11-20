; SYNC : where does the synchronisation of P1 / P8 / T5 (sync.asm, HALT on the GATE ARRAY interrupt) fall on a real
; CPC ? P1 measured on a CRTC 1 (2026-11-18) gives the frame end one microsecond later than Sugarbox, while SHAKER
; E (#03FB, #03FC, photos of the same machine) agrees with Sugarbox : the code after the synchronisation runs one
; microsecond earlier on the real CPC than in Sugarbox.
; The reference here does not depend on a model of the CRTC : the VSYNC pin rises on C0 = 0 of the line C4 = R7,
; read on the PPI port B. For k = 0 to 7, a synchronisation then a read of the PPI k microseconds later than the one
; before : a string of 8 bits, the VSYNC 0 then 1. Two synchronisations :
; - HALT : sync_frame (sync.asm), as the tests ;
; - NOP : EI then a slide of NOP ; the interrupt code at #0038 drops its return address and jumps on, so the time
;   after it depends only on the interrupt.
; Compare the position of the first 1 with Sugarbox (README) : one place to the left = the code runs one
; microsecond earlier on the real CPC. Different between HALT and NOP : the HALT is in question.
; Results : sync_halt, sync_nop (8 bytes, 0 or 1). Included by crtc.asm.

SYNC_LINES equ 249      ; set with the emulator : the 0 -> 1 in the middle of the string (k = 3 -> 4)
SYNC_NOPS  equ 46      ; after sync_frame (HALT)
SYNC_NOPS_SLIDE equ 37  ; after sync_frame_nop : its exit (POP HL, JP, LD, LD) takes 9 us more

macro SYNC_READ k,result,method,nops
   call {method}
   DELAY_LINES SYNC_LINES
   NOPS {nops}+{k}
   ld b,#F5
   in a,(c)             ; I/O on its 4th us
   and 1
   ld ({result}+{k}),a
mend

sync_run:
   call int_patch
   SYNC_READ 0,sync_halt,sync_frame,SYNC_NOPS
   SYNC_READ 1,sync_halt,sync_frame,SYNC_NOPS
   SYNC_READ 2,sync_halt,sync_frame,SYNC_NOPS
   SYNC_READ 3,sync_halt,sync_frame,SYNC_NOPS
   SYNC_READ 4,sync_halt,sync_frame,SYNC_NOPS
   SYNC_READ 5,sync_halt,sync_frame,SYNC_NOPS
   SYNC_READ 6,sync_halt,sync_frame,SYNC_NOPS
   SYNC_READ 7,sync_halt,sync_frame,SYNC_NOPS
   SYNC_READ 0,sync_nop,sync_frame_nop,SYNC_NOPS_SLIDE
   SYNC_READ 1,sync_nop,sync_frame_nop,SYNC_NOPS_SLIDE
   SYNC_READ 2,sync_nop,sync_frame_nop,SYNC_NOPS_SLIDE
   SYNC_READ 3,sync_nop,sync_frame_nop,SYNC_NOPS_SLIDE
   SYNC_READ 4,sync_nop,sync_frame_nop,SYNC_NOPS_SLIDE
   SYNC_READ 5,sync_nop,sync_frame_nop,SYNC_NOPS_SLIDE
   SYNC_READ 6,sync_nop,sync_frame_nop,SYNC_NOPS_SLIDE
   SYNC_READ 7,sync_nop,sync_frame_nop,SYNC_NOPS_SLIDE
   jp int_restore

; As sync_frame, with a slide of NOP in place of HALT. #0038 : POP HL, JP sync_nop_out (the return address dropped)
sync_frame_nop:
   di
   ld a,#E1             ; POP HL
   ld (#0038),a
   ld a,#C3             ; JP sync_nop_out
   ld (#0039),a
   ld hl,sync_nop_out
   ld (#003A),hl
   WAIT_VSYNC_RISE      ; line 240
   DELAY_LINES 10
   ld bc,#7F9D          ; interrupt counter to 0, no interrupt waiting
   out (c),c
   ei
   repeat 3500          ; 52 lines and more
   nop
   rend
sync_nop_out:
   ld a,#C9             ; #0038 back to RET for sync_frame
   ld (#0038),a
   ret

; Result page
sync_show:
   ld hl,sync_txt
   call print_str
   ld hl,sync_halt
   call sync_bits
   ld hl,sync_txt2
   call print_str
   ld hl,sync_nop
   call sync_bits
   ld hl,sync_txt3
   jp print_str
sync_bits:
   ld b,8
.bit:
   ld a,(hl)
   add a,'0'
   call #BB5A
   inc hl
   djnz .bit
   ret

sync_txt:  db 15,2,"SYNC : VSYNC LUE APRES LA SYNCHRO",13,10,13,10,15,1,"HALT : ",0
sync_txt2: db 13,10,"NOP  : ",0
sync_txt3: db 13,10,13,10,"SUGARBOX : 00001111 ET 00001111",0

sync_halt: ds 8
sync_nop:  ds 8
