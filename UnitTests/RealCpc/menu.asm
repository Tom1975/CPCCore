;MENU : list of the real CPC test programs of realcpc.dsk (README.md, part 1). RUN"MENU", a key : BASIC.
   org #4000
   run start
start:
   ld a,1
   call #BC0E           ; SCR SET MODE : mode 1, screen cleared
   ld hl,txt
.print:
   ld a,(hl)
   or a
   jr z,.wait
   call #BB5A
   inc hl
   jr .print
.wait:
   call #BB18           ; KM WAIT KEY
   rst 0

txt: db 15,1,"TESTS SUR VRAI CPC (REALCPC.DSK)",13,10,13,10
     db 15,2,'RUN"CRTC',15,1,"      SYNC, P1, P8 (BIS), FANTOME",13,10,13,10
     db 15,2,'RUN"GA',15,1,"        T1, T4, T2, T3",13,10,13,10
     db 15,2,'RUN"MONITEUR',15,1,"  T5, ECRANS 1 A 8",13,10,13,10
     db 15,2,'RUN"FDC',15,1,"       RQM, SEEK D'UNE PISTE",13,10,13,10
     db 15,2,'RUN"PLUS',15,1,"      P12 (6128 PLUS)",13,10,13,10
     db 15,1,"UNE TOUCHE : BASIC",0
end:
   save 'MENU.BIN',start,end-start,DSK,'realcpc.dsk'
