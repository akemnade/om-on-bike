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


myintr 
       btfsc PIR1,TMR1IF,0
       call tmroverflow
       clrf tmrportbxor,0
       btfss INTCON,INT0IF,0
       bra myintr2
       bcf INTCON,INT0IF,0 
       movlw 0x11
       movwf tmrportbxor,0
myintr2
       btfss INTCON3,INT1IF,0
       bra tmrread
       bcf INTCON3,INT1IF,0
       movlw 0x22
       iorwf tmrportbxor,1,0
tmrread
       tstfsz tmrportbxor,0
       bra tmrreadreal
       return
tmrreadreal
       movff TMR1L,tmrb0
       movff TMR1H,tmrb1
       btfss PIR1,TMR1IF,0
       goto tmrready
       call tmroverflow
       goto tmrread
tmrready 
#ifndef DO_NOT_CHANGE_FSR2
       lfsr FSR2,PULSE_BUF
       movff pulse_pos,FSR2L
#endif
       btfsc tmrportbxor,1,0
       call updtimerdiff
       movlw 0xfa
       movwf INDF2,0
       incf FSR2L,1,0
       movff tmrportbxor,INDF2
       incf FSR2L,1,0
       movff tmrb2,INDF2
       incf FSR2L,1,0
       movff tmrb1,INDF2
       incf FSR2L,1,0
       movff tmrb0,INDF2
       incf FSR2L,1,0
       movlw 0xfb
       movwf INDF2,0
       incf FSR2L,1,0
       movlw 6
       addwf pulse_pos,1,0
       call check_pulse_send
       return

updtimerdiff
      movf tmrold2,0,0
      cpfseq tmrb2,0
      bra updtimerdiff2
      return
updtimerdiff2 
      movf tmrold0,0,0
      movff tmrb0,tmrdiff0
      subwf tmrdiff0,1,0
      movf tmrold1,0,0
      movff tmrb1,tmrdiff1
      subwfb tmrdiff1,1,0
      movf tmrold2,0,0
      movff tmrb2,tmrdiff2
      subwfb tmrdiff2,1,0
      
      movff tmrb2,tmrold2
      movff tmrb1,tmrold1
      movff tmrb0,tmrold0
      return   
   
tmroverflow
       bcf PIR1,TMR1IF,0
       incf tmrb2,1,0
       return

timer_init
        movff PORTB,tmrportbold
        clrf usbpulsecount,0
        clrf pulse_pos,0
        clrf tmrb0,0
        clrf tmrb1,0
        clrf tmrb2,0
        clrf tmrold0,0
        clrf tmrold1,0
        clrf tmrold2,0
        clrf tmrdiff0,0
        clrf tmrdiff1,0
        clrf tmrdiff2,0
        clrf TMR1L,0
        clrf TMR1H,0
        movlw 0x27
        movwf T1CON,0
        movlw 8
        movff WREG,RPINR1
        bcf INTCON2,INTEDG0,0
        bcf INTCON2,INTEDG1,0
        bsf INTCON3,INT1IE,0
        bsf INTCON,INT0IE,0
        bsf PIE1,TMR1IE,0
        ;bsf INTCON,RBIE,0
        bsf INTCON,PEIE,0
        ; bsf INTCON,GIE,0
        return
