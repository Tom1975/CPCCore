; MONITEUR : the monitor tests (T5) of README.md, part 1. RUN"MONITEUR" from realcpc.dsk (built by build.sh).
   include 'common.asm'
   org #4000
   run start
   include 'menucore.asm'
   include 't5mon.asm'

menu_list:
   ld a,'1'
   ld hl,txt_crtc1
   call section
   ld hl,txt_tests1
   jp print_str

; key, test, parameter (A), result page (0 : none)
entries:
   db '1' : dw t5_run : db 1 : dw 0
   db '2' : dw t5_run : db 2 : dw 0
   db '3' : dw t5_run : db 3 : dw 0
   db '4' : dw t5_run : db 4 : dw 0
   db '5' : dw t5_run : db 5 : dw 0
   db '6' : dw t5_run : db 6 : dw 0
   db '7' : dw t5_run : db 7 : dw 0
   db '8' : dw t5_run : db 8 : dw 0
   db 'Q' : dw quit : db 0 : dw 0
   db 0

; Texts : 15,n = pen n ; 13,10 = new line. 39 characters at most per line.
menu_name:  db 15,1,"TESTS DU MONITEUR (CTM644)",0
txt_crtc1:  db 13,10,"CRTC 1",0
txt_tests1: db 15,1," 1  T5  ECRAN 1 : 64 L SANS HSYNC",13,10
            db " 2  T5  ECRAN 2 : 200 L SANS HSYNC",13,10
            db " 3  T5  ECRAN 3 : HSYNC +464, 64 L",13,10
            db " 4  T5  ECRAN 4 : HSYNC +464, 200 L",13,10
            db " 5  T5  ECRAN 5 : 2E HSYNC 32 PX",13,10
            db " 6  T5  ECRAN 6 : PRINCIPALE 32 PX",13,10
            db " 7  T5  ECRAN 7 : HSYNC +112, 200 L",13,10
            db " 8  T5  ECRAN 8 : 2 HSYNC EN PLUS",13,10,0

end:
   ifdef AUTO
   save 'MONITEUR.BIN',start,end-start,DSK,'calib.dsk'
   else
   save 'MONITEUR.BIN',start,end-start,DSK,'realcpc.dsk'
   endif
