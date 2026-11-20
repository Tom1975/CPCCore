; GA : the Gate Array tests of README.md, part 1. RUN"GA" from realcpc.dsk (built by build.sh).
   include 'common.asm'
   org #4000
   run start
   include 'menucore.asm'
   include 't1in.asm'
   include 't2vsync.asm'
   include 't3mode.asm'
   include 't4border.asm'

t1_show:
   ld hl,txt_res_t1
   call print_str
   ld a,(t1_value)
   ld l,a
   ld h,0
   call print_dec
   ld hl,txt_res_t1b
   jp print_str

t2_show:
   ld hl,txt_res_t2
   call print_str
   ld hl,(t2_odd)
   call print_dec
   ld hl,txt_res_t2b
   call print_str
   ld hl,(t2_even)
   jp print_dec

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
   db '1' : dw t1_run : db 0 : dw t1_show
   db '2' : dw t4_run : db 0 : dw 0
   db '3' : dw t2_run : db 0 : dw t2_show
   db '4' : dw t3_run : db 0 : dw 0
   db 'Q' : dw quit : db 0 : dw 0
   db 0

; Texts : 15,n = pen n ; 13,10 = new line. 39 characters at most per line.
menu_name:  db 15,1,"TESTS DU GATE ARRAY SUR VRAI CPC",0
txt_all:    db 13,10,15,2,"TOUS LES CRTC",13,10
            db 15,1," 1  T1  IN &74xx : PPI ET GATE ARRAY",13,10
            db " 2  T4  BORDER ET ENCRE",13,10,13,10,0
txt_crtc1:  db "CRTC 1",0
txt_tests1: db 15,1," 3  T2  VSYNC APRES 17 OU 18 HSYNC",13,10
            db " 4  T3  MODE ET R3.JIT",13,10,0
txt_res_t1:    db 15,2,"T1 : IN &7400",13,10,13,10,15,1,"VALEUR LUE ",0
txt_res_t1b:   db " (76 ATTENDU)",0
txt_res_t2:    db 15,2,"T2 : VSYNC APRES 17 OU 18 HSYNC",13,10,13,10,15,1,"17 HSYNC : ",0
txt_res_t2b:   db 13,10,"18 HSYNC : ",0
   assert $ <= #9000    ; T2 builds its IM 2 table at #9000 (t2vsync.asm)

end:
   ifdef AUTO
   save 'GA.BIN',start,end-start,DSK,'calib.dsk'
   else
   save 'GA.BIN',start,end-start,DSK,'realcpc.dsk'
   endif
