        list p=PIC18F25J50, r=dec
include p18f25j50.inc

	EXTERN _asmtmp
	EXTERN _asmtmp2
        GLOBAL _quickread
SPI_READ_BIT macro bit
    bcf LATA,5,0
    bsf LATA,5,0
    btfsc PORTB,0,0
    bsf INDF0,bit,0
    endm
S_quickread__quickread code

_quickread
    movff FSR0L, _asmtmp
    movff FSR0H, _asmtmp2

  lfsr FSR0, 0x7ff
    movlw 128
qreadloop:
    bsf LATA,2,0
    clrf PREINC0,0
    SPI_READ_BIT(7)
    SPI_READ_BIT(6)
    SPI_READ_BIT(5)
    SPI_READ_BIT(4)
    SPI_READ_BIT(3)
    SPI_READ_BIT(2)
    SPI_READ_BIT(1)
    SPI_READ_BIT(0)

    clrf PREINC0,0
    SPI_READ_BIT(7)
    SPI_READ_BIT(6)
    SPI_READ_BIT(5)
    SPI_READ_BIT(4)
    SPI_READ_BIT(3)
    SPI_READ_BIT(2)
    SPI_READ_BIT(1)
    SPI_READ_BIT(0)

    clrf PREINC0,0
    SPI_READ_BIT(7)
    SPI_READ_BIT(6)
    SPI_READ_BIT(5)
    SPI_READ_BIT(4)
    SPI_READ_BIT(3)
    SPI_READ_BIT(2)
    SPI_READ_BIT(1)
    SPI_READ_BIT(0)

 clrf PREINC0,0
    SPI_READ_BIT(7)
    SPI_READ_BIT(6)
    SPI_READ_BIT(5)
    SPI_READ_BIT(4)
    SPI_READ_BIT(3)
    SPI_READ_BIT(2)
    SPI_READ_BIT(1)
    SPI_READ_BIT(0)
    addlw 0xff
      btfss STATUS,Z,0
    bra qreadloop
    movff _asmtmp2,FSR0H
    movff _asmtmp,FSR0L
    return
    END
