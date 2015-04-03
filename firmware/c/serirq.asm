        list p=PIC18F26J50, r=dec
include p18f26j50.inc
#define EP2IADDR 0x700

	global _serfifowritepos
	global _serfiforeadpos
	global _serfifobuf
	global _tfound
	global _tmr
	global _tmrb2
	global _pulse_pos

S_irqroutines	code
read_ser btfss RCSTA,OERR
         bra read_ser2
         bcf RCSTA,CREN,0
         bsf RCSTA,CREN,0 
         bra irqnext
read_ser2
         movff FSR1L,FSRsavel
         movff FSR1H,FSRsaveh
         lfsr FSR1,_serfifobuf
         movff _serfifowritepos,FSR1L
         movf RCREG,0,0
         movwf INDF1,0
         incf _serfifowritepos,1,0
         bcf _serfifowritepos,6,0
         bcf _serfifowritepos,7,0
         movff FSRsavel, FSR1L
         movff FSRsaveh, FSR1H
         bra irqnext
serirq  
        btfsc PIR1,RC1IF,0       
        bra read_ser

irqnext btfss PIR1,TMR1IF,0
	bra irqnext2
	bcf PIR1,TMR1IF,0
	incf tmr2,1,0
	
irqnext2 clrf tmrportbxor,0
	btfss INTCON,INT0IF,0
	bra irqnext3
	bcf INTCON,INT0IF,0
	movlw 0x11
	;iorwf tmrportbxor,1,0
	;bsf _tfound,0,0
irqnext3 btfss INTCON3,INT1IF,0
	bra irqnext4
	bcf INTCON3,INT1IF,0
	movlw 0x22
	iorwf tmrportbxor,1,0
	bsf _tfound,1,0
irqnext4
tmrloop movff TMR1L,tmr0
	movff TMR1H,tmr1
	btfss PIR1,TMR1IF,0
	bra tmrloopend
	bcf PIR1,TMR1IF,0
	incf tmr2,1,0
	bra tmrloop
tmrloopend
	movf tmrportbxor,0,0
	bz irqnext5
	movff FSR0H,FSRsaveh
	movff FSR0L,FSRsavel
	movlw EP2IADDR/256
	movwf FSR0H,0
	movff _pulse_pos,FSR0L
	movlw 0xfa
	movwf POSTINC0,0
	movff tmrportbxor, POSTINC0
	movff tmr2,POSTINC0
	movff tmr1,POSTINC0
	movff tmr0,POSTINC0
	movlw 0xfb
	movwf POSTINC0,0
	movlw 6
	addwf _pulse_pos,1,0
	movff FSRsaveh,FSR0H
	movff FSRsavel,FSR0L
	btfss tmrportbxor, 1,0
	bra notacho
	movff tmr1,_tmr+1
	movff tmr0,_tmr+0
	movff tmr2,_tmr+2
notacho
irqnext5 movff tmr2, _tmrb2
	retfie 1


	org 808h
	goto serirq
udata_irq  udata 0x50
FSRsavel   res 1
FSRsaveh   res 1
_serfifowritepos res 1
_serfiforeadpos res 1
tmrportbxor res 1
_tfound res 1
tmr0	res 1
tmr1	res 1
tmr2	res 1
_tmr	res 4
_tmrb2  res 1
_pulse_pos res 1
udata_serfifo  udata 0x0a00
_serfifobuf	res 64
	END
