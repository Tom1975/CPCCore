; T1IN : is a read of the PPI also a write to the GATE ARRAY ?
; 40010-simplified, sheet Registers, U401 : A15=0, A14=1, IORQ, M1 high ; neither RD nor WR.
; IN A,(C) with BC=#7400 selects the PPI port A (A11=0) and the GATE ARRAY (A15=0, A14=1) : the PPI puts
; on the bus the PSG register 0, which holds #4C : INKR, bright red. The pen selected is the border.
; Border stays blue : the GATE ARRAY ignores a read. Border red for 3 seconds : it takes the value.
; #4F00 : value read, #4C expected in both cases (it says the PPI did answer).
   include 'common.asm'
   org #4000
start:
   di
   ld bc,#7F10          ; pen : border
   out (c),c
   ld a,#44             ; blue
   out (c),a
   ld bc,#F782          ; PPI port A output
   out (c),c
   ld bc,#F400          ; PSG register 0
   out (c),c
   ld bc,#F6C0
   out (c),c
   ld bc,#F600
   out (c),c
   ld bc,#F44C          ; value #4C
   out (c),c
   ld bc,#F680          ; PSG write
   out (c),c
   ld bc,#F600
   out (c),c
   ld bc,#F792          ; PPI port A input
   out (c),c
   ld bc,#F640          ; PSG read
   out (c),c
   ld bc,#7400          ; the test
   in a,(c)
   ld (#4F00),a
   ld bc,#F782          ; PPI as the firmware expects it
   out (c),c
   ld bc,#F600
   out (c),c
   ld de,150            ; 3 seconds
wait:
   WAIT_VSYNC_RISE
   dec de
   ld a,d
   or e
   jr nz,wait
   ei
   ret
end:
   save 'T1IN.BIN',start,end-start,DSK,'realcpc.dsk'
