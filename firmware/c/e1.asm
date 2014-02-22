        list p=PIC18F25j50, r=dec
include p18f25j50.inc
        CONFIG PLLDIV=2, CPUDIV = OSC4_PLL6, WDTEN = OFF, STVREN = OFF, XINST =OFF, CP0 = OFF, OSC = HSPLL, T1DIG = ON, WPDIS = ON, WPCFG = ON, WPEND = PAGE_0, WPFP = PAGE_7, DSWDTOSC = INTOSCREF, DSWDTEN = OFF

        extern __startup
	org 8192
        goto __startup
        END

