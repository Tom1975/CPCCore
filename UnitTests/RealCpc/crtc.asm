; CRTC : the CRTC tests of README.md, part 1. RUN"CRTC" from realcpc.dsk (built by build.sh).
   include 'common.asm'
   org #4000
   run start
   include 'menucore.asm'
   include 'sync.asm'
   include 'synctest.asm'
   include 'p1last.asm'
   include 'p8vsync.asm'

menu_list:
   ld hl,txt_all
   call print_str
   ld a,'1'
   ld hl,txt_crtc1
   call section
   ld hl,txt_tests1
   jp print_str

; key, test, parameter (A), result page (0 : none)
entries:
   db '0' : dw sync_run : db 0 : dw sync_show
   db '1' : dw p1_run : db 0 : dw p1_show
   db '2' : dw p8_run : db 0 : dw p8_show
   db '3' : dw p8b_run : db 0 : dw p8b_show
   db 'Q' : dw quit : db 0 : dw 0
   db 0

; Texts : 15,n = pen n ; 13,10 = new line. 39 characters at most per line.
menu_name:  db 15,1,"TESTS DU CRTC SUR VRAI CPC",0
txt_all:    db 13,10,15,2,"TOUS LES CRTC",13,10
            db 15,1," 0  SYNC  CALAGE DES PROGRAMMES (HALT)",13,10,13,10,0
txt_crtc1:  db "CRTC 1",0
txt_tests1: db 15,1," 1  P1  ECHANTILLON DERNIERE LIGNE",13,10
            db " 2  P8  COMPTEUR DE LIGNES VSYNC",13,10
            db " 3  P8  BIS : R7 SUR C0, R0=0 PAR US",13,10,0

end:
   ifdef AUTO
   save 'CRTC.BIN',start,end-start,DSK,'calib.dsk'
   else
   save 'CRTC.BIN',start,end-start,DSK,'realcpc.dsk'
   endif
