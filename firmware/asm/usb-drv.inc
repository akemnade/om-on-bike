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

DETACHED equ 0
ATTACHED equ 1
POWERED equ 2
USBDEFAULT equ 3
ADR_PENDING equ 4
ADDRESS_STATE equ 5
CONFIGURED_STATE equ 6
WAIT_SETUP equ 0
CTRL_TRF_TX equ 1
CTRL_TRF_RX equ 2

DEVADR equ 2
#define _BSTALL     0x04                ;Buffer Stall enable
#define _DTSEN      0x08                ;Data Toggle Synch enable
#define _DTSBIT        6
#define _INCDIS     0x10                ;Address increment disable
#define _KEN        0x20                ;SIE keeps buff descriptors enable
#define _DAT0       0x00                ;DATA0 packet expected next
#define _DAT1       0x40                ;DATA1 packet expected next
#define _DTSMASK    0x40                ;DTS Mask
#define _USIE       0x80                ;SIE owns buffer
#define _UCPU       0x00                ;CPU owns buffer
#define DSC_DEV     0x01
#define DSC_CFG     0x02
#define DSC_STR     0x03
#define DSC_INTF    0x04
#define DSC_EP      0x05
#define SETUP_TOKEN 0x0d
	

ATTACHED equ 1
POWERED equ 2
USBDEFAULT equ 3
ADR_PENDING equ 4
ADDRESS_STATE equ 5
CONFIGURED_STATE equ 6
WAIT_SETUP equ 0
CTRL_TRF_TX equ 1
CTRL_TRF_RX equ 2
EP0OSTAT equ 0x400
EP0OCNT equ 0x401
EP0OADRL equ 0x402
EP0OADRH equ 0x403
EP0ISTAT equ 0x404
EP0ICNT  equ 0x405
EP0IADRL equ 0x406
EP0IADRH equ 0x407

EP0SIZE equ 0x20
EP0ADDR equ 0x500
EP0IADDR equ 0x520

CTRL_TRF_BUF equ 0x300

EP1OSTAT equ 0x408
EP1OCNT  equ 0x409
EP1OADRL equ 0x40a
EP1OADRH equ 0x40b

EP1ISTAT equ 0x40c
EP1ICNT  equ 0x40d
EP1IADRL equ 0x40e
EP1IADRH equ 0x40f

EP2OSTAT equ 0x410
EP2OCNT  equ 0x411
EP2OADRL equ 0x412
EP2OADRH equ 0x413

EP2ISTAT equ 0x414
EP2ICNT  equ 0x415
EP2IADRL equ 0x416
EP2IADRH equ 0x417

EP3OSTAT equ 0x418
EP3OCNT  equ 0x419
EP3OADRL equ 0x41a
EP3OADRH equ 0x41b

EP3ISTAT equ 0x41c
EP3ICNT equ 0x41d
EP3IADRL equ 0x41e
EP3IADRH equ 0x41f

EP4OSTAT equ 0x420
EP4OCNT  equ 0x421
EP4OADRL equ 0x422
EP4OADRH equ 0x423


EP4ISTAT equ 0x424
EP4ICNT equ 0x425
EP4IADRL equ 0x426
EP4IADRH equ 0x427

usbdbg macro x
#ifdef USB_DBG_TRF
	movlw x
	call usbdbg_out	
#endif
        endm	
usbinit         
        ; btg PORTB,5,0
        clrf serin_pos,0
 ; self power / bus powered ?
        movlw 0
        movff WREG,UEP0
        movlw (1 << UPUEN)  | (1 << FSEN )
	movff WREG,UCFG
	movlw DETACHED
	movwf usbstate,0
        movlw 255
        movwf oldstate,0
        bsf USBIENABLE
        return

check_bus_state
        bcf USBIFLAG
 ;      btfss usb_bus_sense
 ;	goto cbs1
	btfss UCON,USBEN,0
	call usb_module_enable
	goto cbs2
cbs1    btfsc UCON,USBEN,0
        call usb_module_disable
cbs2    movlw ATTACHED
        subwf usbstate,0
	btfss STATUS,Z,0
	return
	btfsc UCON,SE0,0
	return
        movlb UIE/256
	clrf UIR,0
	clrf UIE,1
	bsf UIE,URSTIE,1
	bsf UIE,IDLEIE,1
	movlw POWERED
	movwf usbstate,0
	return

usb_module_enable
        clrf UCON,0
        movlw 0
        movff WREG,UIE
	bsf UCON,USBEN,0
	movlw ATTACHED
	movwf usbstate,0
	return

usb_module_disable
        clrf UCON,0
        movlw 0
        movff WREG,UIE
	movlw DETACHED
	movwf usbstate,0
        return

usb_driver_service
       movlb UIE/256
       movlw DETACHED
       subwf usbstate,0
       btfsc STATUS,Z,0
       return
       movlw 2
       btfsc UIR,ACTVIF,0
       addlw 0xff
       btfsc UIE,ACTVIE,1
       addlw 0xff
       addlw 0
       btfsc STATUS,Z,0
       call usb_wake_from_suspend
       btfsc UCON,SUSPND,0
       return
       movlw 2
       movlb UIE/256
       btfsc UIR,URSTIF,0
       addlw 0xff
       btfsc UIE,URSTIE,1
       addlw 0xff
       addlw 0
       btfsc STATUS,Z,0
       call usb_protocol_reset_handler
       movlw 2
       movlb UIE/256
       btfsc UIR,IDLEIF,0
       addlw 0xff
       btfsc UIE,IDLEIE,1
       addlw 0xff
       addlw 0
       btfsc STATUS,Z,0
       call usb_suspend
       movlw 2
       btfsc UIR,SOFIF,0
       addlw 0xff
       btfsc UIE,SOFIE,1
       addlw 0xff
       addlw 0
       btfsc STATUS,Z,0
       call usb_sof_handler
       movlw 2
       movlb UIE/256
       btfsc UIR,STALLIF,0
       addlw 0xff
       btfsc UIE,STALLIE,1
       addlw 0xff
       addlw 0
       btfsc STATUS,Z,0
       call usb_stall_handler
       movlw 2
       btfsc UIR,UERRIF,0
       addlw 0xff
       btfsc UIE,UERRIE,1
       addlw 0xff
       addlw 0
       btfsc STATUS,Z,0
       call usb_error_handler
       movf usbstate,0,0
       addlw 256-USBDEFAULT
       btfss STATUS,C,0
       return
       movlb UIE/256
       movlw 2
       btfsc UIR,TRNIF,0
       addlw 0xff
       btfsc UIE,TRNIE,1
       addlw 0xff
       addlw 0
       btfss STATUS,Z,0
       return
       call usb_ctrl_ep_service
       movlw 256-ADDRESS_STATE
       addwf usbstate,0,0
       btfsc STATUS,C
       call usb_other_ep_service
       bcf UIR,TRNIF,0
       return
       
usb_wake_from_suspend
       usbdbg 45h
       bcf UCON,SUSPND,0
       bcf UIE,ACTVIE,0
       bcf UIR,ACTVIF,0
       return

usb_protocol_reset_handler
       usbdbg 44h
       movlb UADDR/256
       clrf UEIR,0
       clrf UIR,0
       movlw 0x9f
       movwf UEIE,1
       movlw 0x3b
       movwf UIE,1
       clrf UADDR,1
       clrf UEP1,1
       clrf UEP2,1
       clrf UEP3,1
       clrf UEP4,1
       clrf UEP5,1
       clrf UEP6,1
       clrf UEP7,1
       clrf UEP8,1
       clrf UEP9,1
       clrf UEP10,1
       clrf UEP11,1
       clrf UEP12,1
       clrf UEP13,1
       clrf UEP14,1
       clrf UEP15,1
       
       movlw (1 << EPHSHK)|( 1<< EPOUTEN)| (1 << EPINEN)
       movwf UEP0,1
uprh1  btfss UIR,TRNIF,0
       bra uprh2
       bcf UIR,TRNIF,0
       bra uprh1
uprh2  bcf UCON,PKTDIS,0
       call usb_prepare_for_next_setup_trf
       movlw USBDEFAULT
       movwf usbstate,0
       return

usb_suspend
       usbdbg 46h
       movlb UIE/256
       bsf UIE,ACTVIE,1
       bcf UIR,IDLEIF,0
       bsf UCON,SUSPND,0
       
       return

usb_sof_handler
       ; movlw 41h
       ; call usbdbg_out
       bcf UIR,SOFIF,0
       return

usb_stall_handler
      usbdbg 42h
       movlb UEP0/256
       btfss UEP0,EPSTALL,1
       bra ush1
       call usb_prepare_for_next_setup_trf
       bcf UEP0,EPSTALL,1
ush1   bcf UIR,STALLIF,0
       return
       
usb_error_handler
      usbdbg 43h
       movf UEIR,0,0
       call usbdbg_out
       clrf UEIR,0
       bcf UIR,UERRIF,0
      
       return
EP00_OUT equ 0
EP00_IN  equ 4
usb_ctrl_ep_service
       movlb 4
       movlw EP00_OUT
       subwf USTAT,0,0
       btfss STATUS,Z,0
       bra uces1
       usbdbg 47h
       movf EP0OSTAT,0,1
       andlw 0x3c
       sublw SETUP_TOKEN*4
       bnz uces2
       call usb_ctrl_trf_setup_handler
       return
uces2  goto usb_ctrl_trf_out_handler
uces1  movlw EP00_IN
       subwf USTAT,0,0
       btfss STATUS,Z,0
       return
       usbdbg 48h
       goto usb_ctrl_trf_in_handler

my_setup_handler
  ; TODO
       return

usb_ctrl_trf_setup_handler
       usbdbg 49h
       movlb 4
       movlf ctrl_trf_state, WAIT_SETUP
       call usb_check_std_request
       call my_setup_handler
       call usb_ctrl_ep_service_complete
       return

usb_ctrl_trf_out_handler
       usbdbg 4ah
       movlb 4
       movlw CTRL_TRF_RX
       subwf ctrl_trf_state,0,0
       btfss STATUS,Z,0
       goto usb_prepare_for_next_setup_trf
       call usb_ctrl_trf_rx_service
       movlb 4
       btfss EP0OSTAT,_DTSBIT
       movlw _USIE|_DAT1|_DTSEN
       btfsc EP0OSTAT,_DTSBIT
       movwf _USIE|_DAT0|_DTSEN
       movwf EP0OSTAT
       usbdbg 4dh
       return

usb_check_adr_pending_state
       movff EP0ADDR+DEVADR,UADDR
       movlb UADDR/256
       movlw 0
       cpfsgt UADDR,1
       bra ucaps1
       movf UADDR,0,1
       call usbdbg_out
       movlf usbstate, ADDRESS_STATE
       goto init_other_eps
       ;return
ucaps1 movlf usbstate, USBDEFAULT
       return

usb_ctrl_trf_in_handler
       usbdbg 4ch
       movf usbstate,0,0
       sublw ADR_PENDING
       btfsc STATUS,Z,0
       call usb_check_adr_pending_state
       movlb 4
       movlw CTRL_TRF_TX
       subwf ctrl_trf_state,0,0
       btfss STATUS,Z,0
       goto usb_prepare_for_next_setup_trf
       call usb_ctrl_trf_tx_service
       ; call ep0idbgout
       movlb 4
       btfss EP0ISTAT,_DTSBIT
       movlw _USIE|_DAT1|_DTSEN
       btfsc EP0ISTAT,_DTSBIT
       movlw _USIE|_DAT0|_DTSEN
       movwf EP0ISTAT
       usbdbg '2'
       return

usb_std_set_cfg_handler
     movlw CONFIGURED_STATE
     movwf usbstate,0
     ; call init_other_eps
     ; TODO
     return

usb_std_get_status_handler
    ;TODO
    movlf wcount,2
    movlf CTRL_TRF_BUF,0
    movlf CTRL_TRF_BUF+1,0
    return
usb_std_feature_req_handler
    ;TODO
    return

usb_check_std_request
       movlb EP0ADDR/256
       movf EP0ADDR,0,1
       andlw 3<<5
       btfss STATUS,Z,0
       return
       movlw 5
       cpfseq EP0ADDR+1,1
       bra ucsr1
       movlf usbstate,ADR_PENDING
       usbdbg 4bh
       return
ucsr1  
       usbdbg 4ch
       movf EP0ADDR+1,0,1
       call usbdbg_out
       movlw 6
       cpfseq EP0ADDR+1,1
       bra uscr2
       goto usb_std_get_dsc_handler
uscr2  movlw 9
       cpfseq EP0ADDR+1,1
       bra uscr3
       goto usb_std_set_cfg_handler
uscr3  movlw 8
       cpfseq EP0ADDR+1,1
       bra uscr4
       movlf EP0IADDR,0
       movlf wcount,1

       ; TODO GetCfg
       return
uscr4  movlw 0
       cpfseq EP0ADDR+1,1
       bra uscr5
       goto usb_std_get_status_handler
uscr5  movlw 1
       cpfseq EP0ADDR+1,1
       bra uscr6
       goto uscr6b
uscr6  movlw 3
       cpfseq EP0ADDR+1,1
       bra uscr7
uscr6b goto usb_std_feature_req_handler
uscr7  movlw 10
       cpfseq EP0ADDR+1,1
       bra uscr8
       movlf wcount,1
       movff EP0ADDR+4,CTRL_TRF_BUF
       ;TODO GET INTF
       return
uscr8  movlw 11
       cpfseq EP0ADDR+1,1
       return
       ; TODO SET INTF
       return
usb_std_get_dsc_handler 
       lfsr FSR0,CTRL_TRF_BUF
       clrf ctrl_trf_pos,0
       usbdbg 4eh
       movlw 80h
       movlb EP0ADDR/256
       cpfseq EP0ADDR
       return
       usbdbg 4fh
       movlw DSC_DEV
       cpfseq EP0ADDR+3
       bra usgdh1
       usbdbg 50h
       settableaddr dev_desc
       movlb CTRL_TRF_BUF/256
       movlf cpcount, dev_desc_size
       call rom2ram
       ; TODO: init device descriptor
       movlf wcount, dev_desc_size
       return
usgdh1 movlw  DSC_CFG
        cpfseq EP0ADDR+3
        bra usgdh2
        settableaddr config_desc
       movlb CTRL_TRF_BUF/256
       movlf cpcount, config_desc_size
       call rom2ram
       ; TODO: init config descriptor
       movlf wcount, config_desc_size
       return 
usgdh2
        settableaddr str_desc_tbl
        movff EP0ADDR+2,cpcount
        movlw 7
        andwf cpcount,1,0
        bz usgdh2_n
usgdh2_l TBLRD*+
         TBLRD*+
         TBLRD*+
         TBLRD*+
        decfsz cpcount,1,0
        bra usgdh2_l
usgdh2_n 
        TBLRD*+
        movff TABLAT, tblutmp
        TBLRD*+
        movff TABLAT, tblhtmp
        TBLRD*+
        movff TABLAT, TBLPTRL
        movff tblutmp, TBLPTRU
        movff tblhtmp, TBLPTRH
        TBLRD*
        movff TABLAT,cpcount
        movff cpcount,wcount
        movlb CTRL_TRF_BUF/256
        call rom2ram
        ; init config descriptor
        ; TODO: 
         return

usb_prepare_for_next_setup_trf
       movlf ctrl_trf_state,WAIT_SETUP
       movlf EP0OCNT,EP0SIZE
       movlf EP0OADRL,EP0ADDR&0xff
       movlf EP0OADRH,EP0ADDR/256
       movlf EP0OSTAT,_USIE ; |_DAT0|_DTSEN
       movlf EP0ISTAT,_UCPU
       return

usb_ctrl_trf_tx_service
       movlb 4
       movf wcount,0,0
       lfsr FSR0,EP0IADDR
       lfsr FSR1,CTRL_TRF_BUF
       movf ctrl_trf_pos,0,0
       addwf FSR1L,1,0
       movlw EP0SIZE
       cpfsgt wcount,0
       bra singletrans
       movlf EP0ICNT,EP0SIZE
       movlf cpcount,EP0SIZE
       movlw 256-EP0SIZE
       addwf wcount,1,0
       movlw EP0SIZE
       addwf ctrl_trf_pos,1,0
       bra uctts1
singletrans
       movff wcount,EP0ICNT
       movff wcount,cpcount
       movf wcount,0,0
       iorlw 0
       btfsc STATUS,Z,0
       return
       movlf wcount,0
       clrf ctrl_trf_pos,0
uctts1 movff POSTINC1,POSTINC0
       decfsz cpcount,1,0
       bra uctts1
       return
usb_ctrl_trf_rx_service
       movf wcount,0,0
       memcpy CTRL_TRF_BUF,EP0ADDR,EP0SIZE 
       return

ep0idbgout
      lfsr FSR1,EP0IADDR
      movff EP0ICNT,lftemp
      movf lftemp,0,0
      iorlw 0
      btfsc STATUS,Z,0
      return
ep0idbgoutl  movf POSTINC1,0,0
      call usbdbg_out
      decfsz lftemp,1,0
      bra ep0idbgoutl
      return
#if 0
      lfsr FSR1,EP0IADDR
      movlf POSTINC1,0x19
      movlf POSTINC1,1
      movlf POSTINC1,0
      movlf POSTINC1,0x02
      movlf POSTINC1,0
      movlf POSTINC1,0
      movlf POSTINC1,0
      movlf POSTINC1,32
      movlf POSTINC1,0xd8
      movlf POSTINC1,0x04
      movlf POSTINC1,0x09
      movlf POSTINC1,0
      movlf POSTINC1,1
      movlf POSTINC1,0
      movlf POSTINC1,0
      movlf POSTINC1,0
      movlf POSTINC1,0
      movlf POSTINC1,1
#endif
      return
usb_ctrl_ep_service_complete
       bcf UCON,PKTDIS,0 
       ; handle no owner
       movlb EP0ADDR/256
       btfss EP0ADDR,7
       bra ucesc1
       movff EP0ADDR+6, wcount
       call usb_ctrl_trf_tx_service
       movlf ctrl_trf_state, CTRL_TRF_TX
       movlf EP0OCNT, EP0SIZE
       movlf EP0OADRL,EP0ADDR&0xff
       movlf EP0OADRH,EP0ADDR/256
       movlf EP0OSTAT, _USIE
       movlf EP0IADRL,EP0IADDR&0xff
       movlf EP0IADRH,EP0IADDR/256
       ; call ep0idbgout
       movlf EP0ISTAT, _USIE|_DAT1|_DTSEN
       usbdbg '1'
       return
ucesc1 movlf ctrl_trf_state, CTRL_TRF_RX 
       movlf EP0ICNT, 0
       movlf EP0ISTAT, _USIE|_DAT1|_DTSEN
       movlf EP0OCNT, EP0SIZE
       movlf EP0OADRL,EP0ADDR&0xff
       movlf EP0OADRH,EP0ADDR/256
       usbdbg '0'
       return
