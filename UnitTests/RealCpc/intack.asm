; INT CRTC 1 (key 6, README part 0) : after which instruction of a chain is the GATE ARRAY interrupt taken, for the
; instructions that end with a memory read ? disquettes_test INTREP (photos 2026-11-19) : a chain of LD A,(nn) takes
; it at 64 65 66 67 on CRTC 1 (with or without M4), 64 65 66 63 on CRTC 2 : the CRTC 1 misses it at one of the 4
; alignments ; INC HL, LD (HL),A, LD A,R, NOP, EX (SP),HL : the same on both (test
; Compendium_27.DISABLED_Crtc1LdANnMissesTheInterruptAtOneAlignment).
; Each measure : sync_frame (HALT on the interrupt, interrupts off), #0038 = POP HL, JP ia_catch, wait up to about
; 30 us before the next interrupt (52 lines later), 0 to 3 NOP of alignment, EI, then a chain of 64 times the same
; instruction. The interrupt returns in the chain : the return address gives the number k of instructions done ;
; value = alignment + k x duration (us from the start of the alignment NOPs), 4 values per instruction. FFFF : no
; interrupt in the chain. Run 2 times (cold, then warm), on CRTC 1 and CRTC 2 ; CRTC 0 without M4 if possible.
; Results : ia_res (8 instructions x 4 alignments). Included by crtc.asm.

IA_LINES equ 50           ; set with the emulator : NOP chain taken about 36 instructions in
IA_NOPS  equ 37
IA_COUNT equ 64

; A chain : 3 NOP of alignment (entry = start - alignment), 64 instructions (repeat after the macro), JP ia_missed
macro IA_NOPS3
   nop
   nop
   nop
mend

ia_run:
   call int_patch
   push ix
   ld ix,ia_data
   ld hl,ia_table
   ld de,ia_res
   ld b,8
.instr:
   push bc
   ld c,4               ; alignments 3, 2, 1, 0 : entry = start - alignment
.align:
   push hl
   push de
   ld a,(hl)            ; chain start
   ld (ia_start),a
   inc hl
   ld a,(hl)
   ld (ia_start+1),a
   inc hl
   ld a,(hl)            ; instruction length (bytes)
   ld (ia_len),a
   inc hl
   ld a,(hl)            ; duration (us)
   ld (ia_dur),a
   ld a,c
   dec a
   ld (ia_align),a
   ld hl,(ia_start)
   ld e,a
   ld d,0
   or a
   sbc hl,de
   ld (ia_entry),hl
   push bc
   call ia_measure
   pop bc
   pop de
   ; result for the alignment c-1 : ia_res + 8 x instr + 2 x (c-1)
   push de
   ld a,(ia_align)
   add a,a
   ld l,a
   ld h,0
   add hl,de
   ld de,(ia_val)
   ld (hl),e
   inc hl
   ld (hl),d
   pop de
   pop hl
   dec c
   jr nz,.align
   ld bc,4
   add hl,bc            ; next table entry
   ex de,hl
   ld bc,8
   add hl,bc            ; next result row
   ex de,hl
   pop bc
   djnz .instr
   pop ix
   jp int_restore

; One measure. Keeps nothing.
ia_measure:
   ld a,#C9             ; RET for sync_frame
   ld (#0038),a
   ld hl,ia_data
   ld de,ia_data
   call sync_frame      ; interrupts off, the same microsecond every time
   ld a,#E1             ; POP HL
   ld (#0038),a
   ld a,#C3             ; JP ia_catch
   ld (#0039),a
   ld hl,ia_catch
   ld (#003A),hl
   DELAY_LINES IA_LINES
   NOPS IA_NOPS
   ld hl,ia_data
   ld de,ia_data
   ld bc,(ia_entry)
   push bc
   ld bc,#7F00          ; harmless BC
   ei
   ret                  ; into the chain (alignment NOPs first)

ia_catch:               ; HL = return address in the chain (the RET of ia_measure is on the stack)
   ld de,(ia_start)
   or a
   sbc hl,de            ; bytes done
   ld a,(ia_len)
   ld c,a
   ld de,0              ; k
.div:
   ld a,h
   or l
   jr z,.done
   ld b,0
   or a
   sbc hl,bc
   jr c,.done
   inc de
   jr .div
.done:
   ; value = alignment + k x duration
   ld a,(ia_dur)
   ld b,a
   ld hl,0
.mul:
   add hl,de
   djnz .mul
   ld a,(ia_align)
   ld e,a
   ld d,0
   add hl,de
   ld (ia_val),hl
   ret

ia_missed:
   di
   ld hl,#FFFF
   ld (ia_val),hl
   ret

; chain start, length, duration
ia_table:
   dw ia_ldann : db 3,4
   dw ia_ldhlnn : db 3,5
   dw ia_ldahl : db 1,2
   dw ia_cphl : db 1,2
   dw ia_ldade : db 1,2
   dw ia_addix : db 3,5
   dw ia_inchl : db 1,2
   dw ia_nop : db 1,1

   IA_NOPS3
ia_ldann:
   repeat IA_COUNT
   ld a,(ia_data)
   rend
   jp ia_missed
   IA_NOPS3
ia_ldhlnn:
   repeat IA_COUNT
   ld hl,(ia_data)
   rend
   jp ia_missed
   IA_NOPS3
ia_ldahl:
   repeat IA_COUNT
   ld a,(hl)
   rend
   jp ia_missed
   IA_NOPS3
ia_cphl:
   repeat IA_COUNT
   cp (hl)
   rend
   jp ia_missed
   IA_NOPS3
ia_ldade:
   repeat IA_COUNT
   ld a,(de)
   rend
   jp ia_missed
   IA_NOPS3
ia_addix:
   repeat IA_COUNT
   add a,(ix+0)
   rend
   jp ia_missed
   IA_NOPS3
ia_inchl:
   repeat IA_COUNT
   inc hl
   rend
   jp ia_missed
   IA_NOPS3
ia_nop:
   repeat IA_COUNT
   nop
   rend
   jp ia_missed

ia_data:  dw 0
ia_start: dw 0
ia_entry: dw 0
ia_len:   db 0
ia_dur:   db 0
ia_align: db 0
ia_val:   dw 0
ia_res:   ds 64

; Result page : one row per instruction, the 4 alignments
ia_show:
   ld hl,ia_txt
   call print_str
   ld hl,ia_names
   ld (ia_name),hl
   ld hl,ia_res
   ld (ia_ptr),hl
   ld b,8
.row:
   push bc
   ld hl,(ia_name)
   call print_str
   inc hl               ; past the 0
   ld (ia_name),hl
   ld b,4
.v:
   push bc
   ld hl,(ia_ptr)
   ld e,(hl)
   inc hl
   ld d,(hl)
   inc hl
   ld (ia_ptr),hl
   ex de,hl
   call print_dec
   ld a,' '
   call #BB5A
   pop bc
   djnz .v
   ld a,13
   call #BB5A
   ld a,10
   call #BB5A
   pop bc
   djnz .row
   ret

ia_name:  dw 0
ia_ptr:   dw 0

ia_txt:   db 15,2,"INT : INSTRUCTION QUI PREND L'INT",13,10
          db "(US, CALAGE 0 1 2 3)",13,10,13,10,15,1,0
ia_names: db "LD A,(NN)    ",0
          db "LD HL,(NN)   ",0
          db "LD A,(HL)    ",0
          db "CP (HL)      ",0
          db "LD A,(DE)    ",0
          db "ADD A,(IX+0) ",0
          db "INC HL       ",0
          db "NOP          ",0
