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

	list p=PIC18F25J50, r=dec
include p18f25j50.inc

  CBLOCK 0x10
      lftemp
      usbstate
      ctrl_trf_state
      ctrl_trf_pos
      cpcount 
      tblltmp
      tblhtmp
      tblutmp
      serin_pos
      wcount
      uartfsrsaveh
      uartfsrsavel
      oldstate
      DELAY3
      DELAY2
      lcdstate
      lcdrow
      lcdtmp
      lcdouttmp
      lcdposread
      ferrcount
      ratenum
      picprogstate
      picprogcmd
      picprogdata
      picprogtmp
      picinbufpos
      pulse_pos
      tmrb0
      tmrb1
      tmrb2
      tmrportbtmp
      tmrportbxor
      tmrportbold
      usbpulsecount
      adstate
      vinh
      vinl
      vctlh
      vctll

      tmrold0
      tmrold1
      tmrold2

      tmrdiff0
      tmrdiff1
      tmrdiff2

ENDC


EP1ADDR  equ 0x540

EP1IADDR equ 0x580

EP2ADDR  equ 0x5c0
EP2ISIZE equ 0x40
EP3ADDR  equ 0x600
EP3IADDR equ 0x640
BKBUF equ 0x680
EP4IADDR equ 0x800
EP4ISIZE equ 0x40
EP4ADDR equ 0x840
CTRL_TRF_BUF equ 0x300
SERIN_BUF equ 0x3c0
PICIN_BUF equ 0x2c0 
PULSE_BUF equ 0x700

#define FOSC 16000000
#define LCD_RS     PORTB,0,0
#define LCD_RS_BIT 0
#define LCD_RW     PORTB,1,0
#define LCD_E1     PORTB,2,0
#define LCD_E2     PORTB,3,0
#define RS232_FLOW PORTC,2,0


#define PIC_MCLR PORTA,3,0
#define PIC_POWER PORTA,2,0
#define PIC_DATA PORTB,7,0
#define PIC_CLK PORTB,6,0
#define PIC_DATAT TRISB,7,0
#define USBIFLAG PIR2,USBIF,0
#define USBIENABLE PIE2,USBIE,0

picmclr macro val
        if (val == 1)
            bcf PIC_MCLR
        else
            bsf PIC_MCLR
        endif
        endm
picpower macro val
        if (val == 1)
            bcf PIC_POWER
        else
            bsf PIC_POWER
        endif
        endm



manynop  macro
        nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	endm
   org 0
       goto 8192
       org  8192

	
#include "mystdmacros.inc"

#include "usb-drv.inc"

;	__CONFIG _CONFIG1L, _PLLDIV_2_1L & _CPUDIV_OSC4_PLL6_1L & _USBDIV_2_1L
;	__CONFIG _CONFIG1H, _FOSC_HSPLL_HS_1H & _FCMEM_OFF_1H & _IESO_OFF_1H
;	__CONFIG _CONFIG2L, _PWRT_ON_2L & _BOR_ON_2L & _BORV_2_2L & _VREGEN_ON_2L
;	__CONFIG _CONFIG2L, _PWRT_ON_2L & _BOR_OFF_2L & _VREGEN_ON_2L
;	__CONFIG _CONFIG2H, _WDT_OFF_2H
;	__CONFIG _CONFIG3H, _MCLRE_OFF_3H
;	__CONFIG _CONFIG4L, _LVP_OFF_4L & _DEBUG_OFF_4L & _XINST_OFF_4L
;	__CONFIG _CONFIG5L, _CP0_OFF_5L & _CP1_OFF_5L & _CP2_OFF_5L & _CP3_OFF_5L
;        __CONFIG _CONFIG5H, _CPB_OFF_5H
;	__CONFIG _CONFIG6L, _WRT0_ON_6L & _WRT1_ON_6L & _WRT2_ON_6L & _WRT3_ON_6L
;	__CONFIG _CONFIG6H, _WRTB_ON_6H & _WRTC_ON_6H & _WRTD_ON_6H
;	__CONFIG _CONFIG7L, _EBTR0_OFF_7L & _EBTR1_OFF_7L & _EBTR2_OFF_7L & _EBTR3_OFF_7L
;	__CONFIG _CONFIG7H, _EBTRB_OFF_7H
        ;banksel OSCCON
	;movlw 72h
	;movwf OSCCON
	;banksel TRISC
main
        bsf OSCCON,IDLEN,0
	movlw 0xb0
	movwf TRISC,0
	movlw 0xf 
	movwf TRISA,0
	movlw 0xb1 
	movwf TRISB,0
        movlw 0x1f
        movff WREG,ANCON1
	movlw 5h
	movwf PORTA,0
	;banksel PORTC
        movlw 00h
        movwf PORTC,0
	movlw 00h
	movwf PORTB,0
	movlw 0x0b
	movwf ADCON1,0
        clrf ADCON0,0
        clrf UCON,0
        ;movlf ADCON2,2             ; TAD=32TOSC < 22Mhz TAD
        bsf ADCON0,ADON,0
        
ser_init
        setbaud 115200
        setbaud2 115200
        ; movlw 34
        ; movwf SPBRG,0
        ; movlw 0
        ;movwf SPBRGH,0
        bsf RCSTA,SPEN,0
        bsf RCSTA2,SPEN,0
        movlw 1 << BRG16
        movwf BAUDCON,0
        movwf BAUDCON2,0
        movlw (1 << BRGH) | (1<< TXEN)
        movwf TXSTA,0
        movwf TXSTA2,0 
        bsf RCSTA,CREN,0
        bsf RCSTA2,CREN,0
        movlw 5
        ; movff WREG,RPOR9
        movlw 10
        ; movff WREG,RPINR16 
        movlw 0
        movwf TXREG,0
        movlw 0
        call usbdbg_out
        movlw 0
        call usbdbg_out
        movlw 255 
        call usbdbg_out
        movlw 255 
        call usbdbg_out
        movlw 255 
        call usbdbg_out
        clrf ratenum,0
        clrf ferrcount,0
        ;call init_lcd
        ;call init_lcd
vinwaitloop
#ifndef NOVINCHECK
        movlw 255
        movwf cpcount,0
        bsf ADCON0,1,0
        waitus 1000
        movff ADRESH,TXREG
        movlw 29
        cpfsgt ADRESH,0
        goto vinwaitloop
#endif
        movlw 5
        movwf wcount,0 
initdll2
       
        movlw 255
        movwf cpcount,0
initdelayloop
        waitus 1000
        waitus 1000
        waitus 1000
        waitus 1000
        decfsz cpcount,1,0
        goto initdelayloop
        decfsz wcount,1,0
        goto initdll2
        clrf picinbufpos
        call proginit
        bcf RS232_FLOW
        call usbinit
	call timer_init
        bsf INTCON,TMR0IE,0
        bsf INTCON,PEIE,0
usbpoll 
        bcf PORTB,1,0
        sleep
        bsf PORTB,1,0
        ;  btg PORTB,5,0		; 
        call check_bus_state
        call usb_driver_service
        btfsc INTCON,TMR0IF,0
        call chk_timer_tx
        movlw CONFIGURED_STATE
        cpfseq usbstate,0
        goto usbpoll2
	 call myintr
        call picdatatx
        call check_pulse_send
usbpoll2
       ; call check_uart_rx
        movf oldstate,0,0
        subwf usbstate,0,0
        btfsc STATUS,Z,0
        goto usbpoll
        movff usbstate,oldstate
        movf usbstate,0,0
        iorlw 0xa0
        call usbdbg_out
        goto usbpoll
chk_timer_tx
        bcf INTCON,TMR0IF,0
	;      call uart2usb_tx	;
	call adstatecheck
	return
adstatecheck
	incf adstate,1,0
	btfsc adstate,0,0 	; bit 0 = 1 -> start_adconv
	bra start_adconv
        btfsc adstate,1,0       ; bit 1 = 0 -> switch to vin, bit 2 = 1 -> switch to vctl
	bra advctlinit
	movff ADRESH,vctlh
	movff ADRESL,vctll
	
	bsf ADCON1,4,0
        movlw 0x01
        movwf ADCON0,0
        return
advctlinit
	movff ADRESH,vinh
	movff ADRESH,vinl
	 bcf ADCON1,4,0
        movlw 0xd
        movwf ADCON0,0
	return
	
start_adconv
	bsf ADCON0,1,0
;	btfsc adstate,1,0
;	movff ADRESH,vinh
;	btfsc adstate,1,0
;	movff ADRESL,vinl
;	btfss adstate,1,0
;	movff ADRESH,vctlh
;	btfss adstate,1,0
;	movff ADRESL,vctll
        return

	
check_pulse_send
        movlb EP2ISTAT/256
        btfsc EP2ISTAT,7
        return
        movlw 256-EP2ISIZE
        andwf pulse_pos,0,0
        subwf usbpulsecount,0,0
        btfsc STATUS,Z,0
        return
        movff usbpulsecount,EP2IADRL
        movlw EP2ISIZE
        addwf usbpulsecount,1,0
        movwf EP2ICNT
        movlw PULSE_BUF/256
        movwf EP2IADRH
        btfss EP2ISTAT,_DTSBIT
        movlw _USIE|_DAT1|_DTSEN
        btfsc EP2ISTAT,_DTSBIT
        movlw _USIE|_DAT0|_DTSEN
        movwf EP2ISTAT
        return

picdatatx
        lfsr FSR1,PICIN_BUF
        lfsr FSR2,EP3IADDR
        movlb EP3ISTAT/256
        btfsc EP3ISTAT,7
        return
        movff picinbufpos,EP3ICNT
        movf picinbufpos,0,0
        iorlw 0
        btfsc STATUS,Z,0
        return
picdatatxloop
        movff POSTINC1,POSTINC2
        decfsz picinbufpos,1,0
        goto picdatatxloop
        movlw EP3IADDR/256
        movwf EP3IADRH
        movlw EP3IADDR&255
        movwf EP3IADRL
        btfss EP3ISTAT,_DTSBIT
        movlw _USIE|_DAT1|_DTSEN
        btfsc EP3ISTAT,_DTSBIT
        movlw _USIE|_DAT0|_DTSEN
        movwf EP3ISTAT
 
        return

picdatareturn
        lfsr FSR1,PICIN_BUF
        movwf lftemp,0
        movf picinbufpos,0,0
        addwf FSR1L,1,0
        movff lftemp,INDF1
        incf picinbufpos,1,0
        movff EP3ISTAT,lftemp
        movf lftemp,0,0
        btfss lftemp,7,0
        call picdatatx
        movlw 0x3f
        andwf picinbufpos,1,0
        return

uart2usb_tx 
        movff FSR0L,uartfsrsavel
        movff FSR0H,uartfsrsaveh
        lfsr FSR0,EP1IADDR
        lfsr FSR2,SERIN_BUF
        movf serin_pos,0,0
        iorlw 0
        btfsc STATUS,Z,0
        goto uart2usbloopend
        movff EP1ISTAT,lftemp
        btfsc lftemp,7
        goto uart2usbloopend
        movff serin_pos,EP1ICNT
uart2usbloop
        movff POSTINC2,POSTINC0
        decfsz serin_pos,1,0
        goto uart2usbloop
        movlf EP1IADRL,EP1IADDR&255
        movlf EP1IADRH,EP1IADDR/256
        movlb EP1ISTAT/256
        btfss EP1ISTAT,_DTSBIT
        movlw _USIE|_DAT1|_DTSEN
        btfsc EP1ISTAT,_DTSBIT
        movlw _USIE|_DAT0|_DTSEN
        movwf EP1ISTAT
uart2usbloopend
        movff uartfsrsavel,FSR0L
        movff uartfsrsaveh,FSR0H
        bcf INTCON,TMR0IF,0
        return

nousb_uart
        btfss RCSTA,OERR,0
        goto sernooverrun
        bcf RCSTA,CREN,0
        bsf RCSTA,CREN,0
        return
sernooverrun
        btfsc PIR1,RCIF,0
        goto serdata
        bcf RS232_FLOW
        return
serdata  btfsc RCSTA,FERR,0
        incf ferrcount,1,0
        btfsc ferrcount,2,0
        call change_rate
        movf RCREG,0,0
        bsf RS232_FLOW
       ; call write_lcd
        return
change_rate
        btg ratenum,0,0
        btfsc ratenum,0,0
        call change_115200
        btfss ratenum,0,0
        call change_4800
        clrf ferrcount
        return

change_115200
        setbaud 115200
        return

change_4800
        setbaud 4800
        return


check_uart_rx
        movlw CONFIGURED_STATE-1
        cpfsgt usbstate,0
        goto nousb_uart
        btfsc UCON,SUSPND,0
        goto nousb_uart

        bcf RS232_FLOW

        btfss PIR1,RCIF,0
        return
        movlw 'e'
        call usbdbg_out
        lfsr FSR2,SERIN_BUF
        movf serin_pos,0,0
        addwf FSR2L,1,0
        btfss STATUS,C
        goto cur1
        call uart2usb_tx
        clrf serin_pos
        lfsr FSR2,SERIN_BUF
cur1    movff RCREG,POSTINC2
        incf serin_pos,1,0
        goto check_uart_rx
        
usb_other_ep_service
        movlw 'b'
        call usbdbg_out
        movlb EP1OSTAT/256
        btfss EP1OSTAT,7
        call ep1_service
        btfss EP2OSTAT,7
        call ep2_service
        btfss EP3OSTAT,7
        call ep3_service
        btfss EP4OSTAT,7
        call ep4_service
        return
ep4_service
        lfsr FSR0,EP4ADDR
        movff EP4OCNT,cpcount
        movf cpcount,0,0
        iorlw 0
        btfsc STATUS,Z,0
        goto ep3_endcploop
ep4_cploop
        movf POSTINC0,0,0
        call sec_rs232_out
        decfsz cpcount,1,0
        goto ep4_cploop
ep4_endcploop
        call init_ep4_desc
        return 
ep3_service
        lfsr FSR0,EP3ADDR
        movff EP3OCNT,cpcount
        movf cpcount,0,0
        iorlw 0
        btfsc STATUS,Z,0
        goto ep3_endcploop
ep3_cploop
        movf POSTINC0,0,0
        call handle_pic_action
        decfsz cpcount,1,0
        goto ep3_cploop
ep3_endcploop
        call init_ep3_desc
        return

ep2_service
        return
        lfsr FSR0,EP2ADDR
        movff EP2OCNT,cpcount
        movf cpcount,0,0
        iorlw 0
        btfsc STATUS,Z,0
        goto ep2_endcploop
ep2_cploop
        movf POSTINC0,0,0
        ; call write_lcd
        decfsz cpcount,1,0
        goto ep2_cploop
ep2_endcploop
        call init_ep2_desc
        return
ep1_service
        lfsr FSR0,EP1ADDR
        movff EP1OCNT,cpcount
        movf cpcount,0,0
        iorlw 0
        btfsc STATUS,Z,0
        goto ep1_endcploop
ep1_cploop 
        movlw 0x30
	cpfseq INDF0,0
	bra acctrl0
        movlb EP1ISTAT/256
        btfsc EP1ISTAT,7
        bra ep1_endl
        movlw 0x31

        movlb BKBUF/256
        movwf BKBUF,1 
        movff vinh,BKBUF+1
        movff vinl,BKBUF+2
	movff vctlh,BKBUF+3
	movff vctll,BKBUF+4
        movff tmrdiff0,BKBUF+8
        movff tmrdiff1,BKBUF+7
        movff tmrdiff2,BKBUF+6
        clrf BKBUF+5,1
	
        movlb EP1ISTAT/256 
        movlw BKBUF/256
        movwf EP1IADRH,1
        movlw BKBUF&0xff
        movwf EP1IADRL,1
        movlw 1+4+4
        movwf EP1ICNT,1
        btfss EP1ISTAT,_DTSBIT
        movlw _USIE|_DAT1|_DTSEN
        btfsc EP1ISTAT,_DTSBIT
        movlw _USIE|_DAT0|_DTSEN
        movwf EP1ISTAT
	bra cpendl
acctrl0
        movlw 0x61
        cpfseq INDF0,0
        bra acctrl1
        bsf ADCON1,4,0
        movlw 0x01
        movwf ADCON0,0
        bra cpendl
acctrl1 movlw 0x62 
        cpfseq INDF0,0 
        bra acctrl2
        bsf ADCON1,4,0
        movlw 0x05
        movwf ADCON0,0
        bra cpendl
acctrl2 movlw 0x63
        cpfseq INDF0,0
        bra acctrl3
        bsf ADCON1,4,0
        movlw 0x09
        movwf ADCON0,0
        bra cpendl
acctrl3 movlw 0x64
        cpfseq INDF0,0
        bra acctrl4
        bcf ADCON1,4,0
        movlw 0xd
        movwf ADCON0,0
        bra cpendl
acctrl4 movlw 0x65
        cpfseq INDF0,0
        goto acctrl5
        ;clrf GIE,0
        clrf UCON,0 
        reset
        goto 0
cpendl
        waitus 800
        bsf ADCON0,1,0
        goto ep1_endl
 
acctrl5 movlw 0x41
        cpfseq INDF0,0
        bra acctrl6
        movlb EP1ISTAT/256
        btfsc EP1ISTAT,7
        bra ep1_endl
        movlw 0x41
        movlb BKBUF/256
        movwf BKBUF,1 
        movff ADRESH,BKBUF+1
        movff ADRESL,BKBUF+2
        movlb EP1ISTAT/256 
        movlw BKBUF/256
        movwf EP1IADRH,1
        movlw BKBUF&0xff
        movwf EP1IADRL,1
        movlw 3
        movwf EP1ICNT,1
        btfss EP1ISTAT,_DTSBIT
        movlw _USIE|_DAT1|_DTSEN
        btfsc EP1ISTAT,_DTSBIT
        movlw _USIE|_DAT0|_DTSEN
        movwf EP1ISTAT

        bsf ADCON0,1,0
ep1_endl
        movf POSTINC0,0,0
        decfsz cpcount,1,0
        bra ep1_cploop
ep1_endcploop
        movf POSTINC0,0,0
        call init_ep1_desc 
        return

acctrl6 movlw 0x41
        cpfsgt INDF0,0
        bra ep1_endl
        movlw 0x4a
        cpfslt INDF0,0
        bra ep1_endl 
        movlw 0x100-0x42
        addwf INDF0,1,0
        movlw HIGH acctrl_calltable
        movwf PCLATH,0
        movf INDF0,0,0
        addwf INDF0,0,0
        addwf INDF0,0,0
        addwf INDF0,0,0
        addwf PCL,1,0
acctrl_calltable
        bsf PORTB,2,0 
        bra ep1_endl
        bcf PORTB,2,0 
        bra ep1_endl
        bsf PORTB,1,0 
        bra ep1_endl
        bcf PORTB,1,0 
        bra ep1_endl
        bsf PORTC,0,0 
        bra ep1_endl
        bcf PORTC,0,0 
        bra ep1_endl
        bsf PORTC,2,0 
        bra ep1_endl
        bcf PORTC,2,0 
        bra ep1_endl

        
        return
        
        

sec_rs232_out 
      btfss PIR3,TX2IF,0
      goto sec_rs232_out
      movwf TXREG2,0
      return

rs232_out 
       call check_uart_rx
       btfss PIR1,TXIF,0
       goto rs232_out
       movwf TXREG,0
       return
usbdbg_out 
#ifdef DBG_USB_TRF
       btfss PIR1,TXIF,0
       goto usbdbg_out
       movwf TXREG,0
#endif
       return







dev_desc db dev_desc_end-dev_desc,1 ; size, dev desc
         db 0x00,0x02    ; USB Spec Release Number
         db 0,0       ; class,subclass
         db 0,32         ; protocol, max packet size
         db 0xD0,0x16 ; Vendor ID
         db 0xd9,0x04 ; Product ID
         db 0x03,0x00    ; Device release
         db 0x01,0x02    ; manf string, product string
         db 0x00,0x01    ; serial number string, num of configurations
dev_desc_end
dev_desc_size equ (dev_desc_end-dev_desc)

config_desc db 9,2
            db 5*9+7*7,0 ;(config_end-config_desc),(config_end-config_desc)/256
            db 0x04,0x01   ; num interfaces, configuration value
            db 0x00,0x80   ; configuration string, attributes
            db 50, 9   ; attributes, power consumption
config_desc_end
if0_desc    ; db if0_desc_end-if0_desc
            db 0x04,0x00   ; interface number
            db 0x00,0x02   ; alternate setting, num_endpoints
            db 0xff,0xc3   ; interface class, subclass
            db 0x00,3  ; interface protocol,interface string
if0_desc_end 

ep1_desc    db 7,0x05
            db 0x01,0x02   ; ep address, bulk transfer
            db 0x10,0x0 ; max_packet_size
            db 0x0,7   ; interval
            db 0x05,0x81
            db 0x02,0x10
            db 0x0,0x0

ep1_desc_end

if1_desc    db 9,4
            db 1,0
            db 1,0xff
            db 0xc2,0
            db 4,7
ep2_desc    db 5,0x82
            db 0x02,0x40
            db 0,0

ep2_end
if2_decs    db 9,4
            db 2,0
            db 2,0xff
            db 0xc1,0
            db 5,7
ep3_desc    db 5,0x03
            db 0x2,0x40
            db 0,0
            db 7,5
            db 0x83,0x2
            db 0x40,0
if3_desc    db 0,9
            db 4,3
            db 0,2 
            db 0xff,0xc0
            db 0,7
ep4_desc    db 7,5
            db 0x4,2
            db 64,0
            db 0,7
            db 5,0x84
            db 2,64
            db 0,0
       
config_end
config_desc_size equ (config_end-config_desc)
str_desc_tbl db UPPER str0_desc, HIGH str0_desc, LOW str0_desc
            db UPPER str1_desc, HIGH str1_desc, LOW str1_desc
            db UPPER str2_desc, HIGH str2_desc, LOW str2_desc
            db UPPER str3_desc, HIGH str3_desc, LOW str3_desc
            db UPPER str4_desc, HIGH str4_desc, LOW str4_desc
            db UPPER str5_desc, HIGH str5_desc, LOW str5_desc
            db UPPER str6_desc, HIGH str6_desc, LOW str6_desc
            db UPPER str7_desc, HIGH str7_desc, LOW str7_desc
str0_desc  db 4,0x3, 0x09,0x04
str1_desc  db str1_desc_end-str1_desc,0x3, 'A',0,'K',0,' ',0
           db 'H',0,'o',0,'b',0,'b',0,'y',0,' ',0
           db 'p',0,'r',0,'o',0,'d',0,'u',0,'c',0,'t',0,'s',0
str1_desc_end
str2_desc  db str2_desc_end-str2_desc,0x3
           db 'B',0,'i',0,'k',0,'e',0,' ',0,'s',0,'e',0,'n',0,'s',0,'o',0,'r',0,' ',0,'s',0,'y',0,'s',0,'t',0,'e',0,'m',0
str2_desc_end 
str3_desc  db str3_desc_end-str3_desc,0x3
           db 'B',0,'i',0,'k',0,'e',0,' ',0,'c',0,'o',0,'n',0,'t',0,'r',0,'o',0,'l',0
str3_desc_end
str4_desc  db str4_desc_end-str4_desc,0x3
           db 'P',0,'u',0,'l',0,'s',0,'e',0,' ',0,'m',0,'e',0,'a',0,'s',0,'u',0,'r',0,'e',0,'m',0,'e',0,'n',0,'t',0,' ',0,'t',0,'i',0,'m',0,'e',0,'r',0
str4_desc_end
str5_desc  db str5_desc_end-str5_desc,0x3
           db 'I',0,'2',0,'C',0,'-',0,'M',0,'a',0,'s',0,'t',0,'e',0,'r',0
str5_desc_end
str6_desc  db str6_desc_end-str6_desc,0x3
           db 'K',0,'e',0,'y',0,'b',0,'o',0,'a',0,'r',0,'d',0
str6_desc_end
str7_desc  db str7_desc_end-str7_desc,0x3
           db 'R',0,'S',0,'2',0,'3',0,'2',0
str7_desc_end






init_ep1_desc
       movlw 'a'
       call usbdbg_out
       bcf RCSTA,CREN,0
       bsf RCSTA,CREN,0
       movlb EP1OCNT/256
       movlw 0x40
       movwf EP1OCNT
       movlw EP1ADDR&255
       movwf EP1OADRL
       movlw EP1ADDR/256
       movwf EP1OADRH
       movlw _USIE
       movwf EP1OSTAT
       return
init_ep2_desc
       movlb EP2OCNT/256
       movlw 0x40
       movwf EP2OCNT
       movlw EP2ADDR&255
       movwf EP2OADRL
       movlw EP2ADDR/256
       movwf EP2OADRH
       movlw _USIE
       movwf EP2OSTAT
       return
init_ep3_desc movlb EP3OCNT/256
              movlw 0x40
              movwf EP3OCNT
              movlw EP3ADDR&255
              movwf EP3OADRL
              movlw EP3ADDR/256
              movwf EP3OADRH
              movlw _USIE
              movwf EP3OSTAT
              return
init_ep4_desc movlb EP4OCNT/256
              movlw 0x40
              movwf EP4OCNT
              movlw EP4ADDR&255
              movwf EP4OADRL
              movlw EP4ADDR/256
              movwf EP4OADRH
              movlw _USIE
              movwf EP4OSTAT
              return
init_other_eps
       movlw (1 << EPCONDIS ) |(1 << EPHSHK)|( 1<< EPOUTEN)|(1 << EPINEN)
       movff WREG,UEP1
       movlf EP1ISTAT,_UCPU | _DAT1
       clrf serin_pos,0
       call init_ep1_desc
       movlw 47h
       movwf T0CON,0
       clrf TMR0L
       bsf T0CON,7
       movlw (1 << EPCONDIS ) |(1<< EPHSHK) | (1 << EPINEN) 
       movff WREG,UEP2
       movlf EP2ISTAT,_UCPU | _DAT1
       ;call init_ep2_desc
       movlw (1 << EPCONDIS ) |(1 << EPHSHK)|( 1<< EPOUTEN)|(1 << EPINEN)
       movff WREG,UEP3
       movlf EP3ISTAT,_UCPU | _DAT1
       call init_ep3_desc
       movlw (1 << EPCONDIS ) |(1 << EPHSHK)|( 1<< EPOUTEN)|(1 << EPINEN)
       movff WREG,UEP4
       movlf EP3ISTAT,_UCPU | _DAT1
       call init_ep4_desc 
        return

#define I2CPROG
include "progmode.inc"
include "timerread-bike.inc"
	manynop
	manynop
	end
