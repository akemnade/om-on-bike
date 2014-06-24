        list p=PIC18F25j50, r=dec
include p18f25j50.inc
        CONFIG PLLDIV=2, CPUDIV = OSC3_PLL3, WDTEN = OFF, STVREN = OFF, XINST =OFF, CP0 = OFF, OSC = HSPLL, T1DIG = ON, WPDIS = ON, WPCFG = ON, WPEND = PAGE_0, WPFP = PAGE_1, DSWDTOSC = INTOSCREF, DSWDTEN = OFF

        extern __startup
	org 2048
        goto __startup
        END

