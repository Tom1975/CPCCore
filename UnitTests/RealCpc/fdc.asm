; FDC : the FDC tests of README.md, part 1. RUN"FDC" from realcpc.dsk (built by build.sh).
   include 'common.asm'
   org #4000
   run start
   include 'menucore.asm'
   include 'fdcrqm.asm'
   include 'seek1.asm'

menu_list:
   ld hl,txt_all
   jp print_str

; key, test, parameter (A), result page (0 : none)
entries:
   db '1' : dw fdc_run : db 0 : dw fdc_show
   db '2' : dw sk_run : db 0 : dw sk_show
   db 'Q' : dw quit : db 0 : dw 0
   db 0

; Texts : 15,n = pen n ; 13,10 = new line. 39 characters at most per line.
menu_name:  db 15,1,"TESTS DU FDC SUR VRAI CPC",0
txt_all:    db 13,10,15,2,"TOUS LES CRTC, DISQUETTE DANS A",13,10
            db 15,1," 1  RQM ENTRE OCTETS DE RESULTAT",13,10
            db " 2  SEEK D'UNE PISTE (SRT A)",13,10,0

end:
   ifdef AUTO
   save 'FDC.BIN',start,end-start,DSK,'calib.dsk'
   else
   save 'FDC.BIN',start,end-start,DSK,'realcpc.dsk'
   endif
