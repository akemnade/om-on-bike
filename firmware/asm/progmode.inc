; 
; om-on-bike - Copyright (C) 2009 - Andreas Kemnade
; This program is free software; you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation; either version 3, or (at your option)
; any later version.
;             
; This program is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied
; warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.

proginit      clrf picprogstate
              return


handle_pic_action  ; call pic_dbg_out
                  btfsc picprogstate,0,0
                  goto progwriteavail
                  ;decf picprogstate,1,0
                  ;btfsc STATUS,Z,0
                  ;goto progreadavail


progrxavail1    bcf picprogstate,0,0 
                movwf picprogcmd,0
		btfss picprogcmd,7,0
                goto progwait
		btfsc picprogcmd,6,0
                picmclr 0
		btfss picprogcmd,6,0
                picmclr 1
                btfsc picprogcmd,5,0
                bsf PIC_DATAT
		btfss picprogcmd,5,0
#ifdef I2CPROG
                bcf PIC_DATA
#else
		bcf PIC_DATAT
#endif
		btfsc picprogcmd,5,0
		goto progreadbyte
		btfsc picprogcmd,4,0
		goto progwritebyte
progsbyte
		btfsc picprogcmd,0,0
#ifdef I2CPROG
		bsf PIC_DATAT
#else
		bsf PIC_DATA
#endif
		btfss picprogcmd,0,0
#ifdef I2CPROG
		bcf PIC_DATAT
#else
                bcf PIC_DATA 
#endif
		btfsc picprogcmd,1,0
		bsf PIC_CLK
		btfss picprogcmd,1,0
		bcf PIC_CLK
	        btfsc picprogcmd,2,0
                picpower 1
	        btfss picprogcmd,2,0
                picpower 0
		
		movlw 0
		btfss picprogcmd,5,0
                return
		btfsc PIC_DATA
		addlw 1
                call picdatareturn
                return
progwritebyte   
                bsf picprogstate,0,0
                return
progwriteavail  
		movwf picprogdata,0
                movlw 15 
                andwf picprogcmd,1,0
		btfss STATUS,Z,0
                goto progwriteloop
                bcf picprogstate,0,0
                return
progwriteloop   btfsc picprogdata,0,0
                bsf PIC_DATAT
		btfss picprogdata,0,0
		bcf PIC_DATAT
		nop
		nop
		nop
		nop
		bsf PIC_CLK
		nop
		nop
		nop
		nop
		nop
		nop
		bcf PIC_CLK
		nop
		nop
		nop
		nop
		nop
		nop
		bcf PIC_DATA
		rrncf picprogdata,1,0
		decfsz picprogcmd,1,0
		goto progwriteloop
                bcf picprogstate,0,0
                return

progreadbyte    btfss picprogcmd,4,0
                goto progsbyte
                movlw 15
                andwf picprogcmd,1,0
		btfsc STATUS,Z,0
		return
                btfss picprogcmd,3,0
                goto picread
                btfss picprogcmd,1,0
                goto picread
                btfss picprogcmd,0,0
                bcf picprogstate,1,0
                btfsc picprogcmd,0,0 
                bsf picprogstate,1,0
                return 
picread		movf picprogcmd,0,0
		;addlw (256-14)
		;btfsc STATUS,Z
		;goto 0
		clrf picprogdata,0
		clrf picprogtmp,0
		incf picprogtmp,1,0
		movlw 1
                btfsc picprogstate,1,0
                bra i2creadloop
progreadloop    bsf PIC_CLK
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		bcf PIC_CLK
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		movf picprogtmp,0,0
		btfsc PIC_DATA
		iorwf picprogdata,1,0
		addwf picprogtmp,1,0
		decfsz picprogcmd,1,0
		goto progreadloop
		movf picprogdata,0,0
                call picdatareturn
                return
i2creadloop
                bsf PIC_CLK
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		movf picprogtmp,0,0
		btfsc PIC_DATA
		iorwf picprogdata,1,0
		addwf picprogtmp,1,0
		bcf PIC_CLK
		nop
		nop
		nop
		nop
		nop
		nop
		decfsz picprogcmd,1,0
		goto i2creadloop
		movf picprogdata,0,0
                call picdatareturn
progwait        movlw 0
                cpfseq picprogcmd,0
                goto prgw1
                waitus 260
                return
prgw1           movlw 1
                cpfseq picprogcmd,0
                goto prgw2
                waitus 100
prgw2           movlw 2
                cpfseq picprogcmd,0
                goto prgw3
                waitus 1000
prgw3           
                return
