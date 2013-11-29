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

	;;  include after goto main
	
#define SPBRGVAL -1+FOSC/4/
setbaud macro rate
         movlw (SPBRGVAL rate)/256
         movwf SPBRGH,0
         movlw (SPBRGVAL rate)&255
         movwf SPBRG,0
         endm

setbaud2 macro rate
         movlw (SPBRGVAL rate)/256
         movwf SPBRGH2,0
         movlw (SPBRGVAL rate)&255
         movwf SPBRG2,0
         endm


movlf  macro adr,val
         movlw val
         movff WREG,adr
	 endm


memcpy macro dest,src,len
         if (len >0)
           movff src,dest
           memcpy dest+1,src+1,len-1
         endif
         endm

settableaddr macro addr
       movlw LOW addr
       movwf TBLPTRL,0
       movlw HIGH addr
       movwf TBLPTRH,0
       movlw UPPER addr
       movwf TBLPTRU,0
       endm


addff    macro src,dst
         movf src,0,0
         addwf dst,1,0
         endm
addffc   macro src,dst
         movf src,0,0
         addwfc dst,1,0
         endm

              goto main
mdelay          movwf DELAY3,0      ;  DELAY3*4+2 , mit call: w*4+4
mdelayloop      nop                ; 3 + (DELAY3-1)*4+2=DELAY3*4+1
                decfsz DELAY3,1,0
                goto mdelayloop
                return
  
mdelay2        movwf DELAY2,0        ; t=DELAY2*1029+4
mdelay2loop    movlw 0xff          ; 6+(255*4+4)+(DELAY2-1)*((255*4+4)+5)
               movwf DELAY3,0       ; =6+1024+(DELAY2-1)*(1024+5)
               call mdelay         ; = 1+(1024+5)*DELAY2
               decfsz DELAY2,1,0     ; = DELAY2*1029+1
               goto mdelay2loop
              return
waitcycles macro cycles
  if (cycles >= 1034)
    movlw (cycles-(4+1))/1029
    call mdelay2
    waitcycles cycles-1029*((cycles-(4+1))/1029)-5
  else
  if (cycles > 1005)
    movlw 250
    call mdelay
    waitcycles cycles-1005
  else
  if (cycles > 9) 
    movlw (cycles-(4+1))/4
    call mdelay
    waitcycles cycles-4*((cycles-(4+1))/4)-5
  else
  if (cycles > 0) 
    nop
    waitcycles cycles-1
  endif
  endif
  endif
  endif
  endm
waitus macro us
  waitcycles (us*1000)/(1000000/(FOSC/4000))
  endm
rom2ram movff cpcount,lftemp
rom2raml
       TBLRD*+
        movff TABLAT, POSTINC0
       decfsz cpcount,1,0
       goto rom2raml
;       lfsr FSR1,CTRL_TRF_BUF
       
;       goto ep0idbgoutl
       return
