        list p=PIC18F26j50, r=dec
include p18f26j50.inc
        CONFIG PLLDIV=2, CPUDIV = OSC3_PLL3, WDTEN = OFF, STVREN = OFF, XINST =OFF, CP0 = OFF, OSC = HSPLL, T1DIG = ON, WPDIS = ON, WPCFG = ON, WPEND = PAGE_0, WPFP = PAGE_1, DSWDTOSC = INTOSCREF, DSWDTEN = OFF

 
        extern __startup
        org 8
        goto 2048+8
	org 2048
        goto __startup
        END

