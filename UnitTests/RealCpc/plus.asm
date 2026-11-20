; PLUS : the 6128 Plus tests of README.md, part 1. RUN"PLUS" from realcpc.dsk (built by build.sh).
; org #8000 : the tests page the ASIC registers in #4000-#7FFF.
   include 'common.asm'
   org #8000
   run start
   include 'menucore.asm'
   include 'p12dma.asm'
   include 'p13dma.asm'

menu_list:
   ld hl,txt_all
   jp print_str

; key, test, parameter (A), result page (0 : none)
entries:
   db '1' : dw p12_run : db 0 : dw p12_show
   db '2' : dw p13_run : db 0 : dw p13_show
   db 'Q' : dw quit : db 0 : dw 0
   db 0

; Texts : 15,n = pen n ; 13,10 = new line. 39 characters at most per line.
menu_name:  db 15,1,"TESTS DU 6128 PLUS SUR VRAI CPC",0
txt_all:    db 13,10,15,2,"6128 PLUS SEULEMENT",13,10
            db 15,1," 1  P12 PAUSE DMA A 1, 2, 3 CANAUX",13,10
            db " 2  P13 INT DMA DU CANAL 2 (IM 2)",13,10,0

end:
   assert end <= #A600
   save 'PLUS.BIN',start,end-start,DSK,'realcpc.dsk'
