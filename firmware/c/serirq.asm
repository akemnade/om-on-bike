        list p=PIC18F26J50, r=dec
include p18f26j50.inc

	global _serfifowritepos
	global _serfiforeadpos
	global _serfifobuf
S_irqroutines	code
read_ser btfss RCSTA,OERR
         bra read_ser2
         bcf RCSTA,CREN,0
         bsf RCSTA,CREN,0 
         bra irqend
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
         bra irqend
serirq  
        btfsc PIR1,RC1IF,0       
        bra read_ser

irqend  
	retfie 1


	org 808h
	goto serirq
udata_irq  udata 0x50
FSRsavel   res 1
FSRsaveh   res 1
_serfifowritepos res 1
_serfiforeadpos res 1

udata_serfifo  udata 0x0a00
_serfifobuf	res 64
	END
