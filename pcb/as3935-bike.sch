v 20110115 2
C 40000 40000 0 0 0 title-B.sym
C 41700 47100 1 0 0 as3935.sym
{
T 41695 47300 5 10 0 1 0 0 1
footprint=QFN16_4_EP
T 42195 47100 5 10 1 1 0 0 1
refdes=U200
T 41695 47098 5 10 0 1 0 0 1
device=AS3935
}
C 41100 45000 1 0 0 capacitor-1.sym
{
T 41300 45700 5 10 0 0 0 0 1
device=CAPACITOR
T 41300 45500 5 10 1 1 0 0 1
refdes=C202
T 41300 45900 5 10 0 0 0 0 1
symversion=0.1
T 41100 45000 5 10 0 0 0 0 1
footprint=0603
}
C 41100 50400 1 270 0 capacitor-1.sym
{
T 41800 50200 5 10 0 0 270 0 1
device=CAPACITOR
T 41600 50200 5 10 1 1 270 0 1
refdes=C200
T 42000 50200 5 10 0 0 270 0 1
symversion=0.1
T 41100 50400 5 10 0 0 0 0 1
footprint=0603
}
C 40800 48000 1 90 0 capacitor-1.sym
{
T 40100 48200 5 10 0 0 90 0 1
device=CAPACITOR
T 40300 48200 5 10 1 1 90 0 1
refdes=C201
T 39900 48200 5 10 0 0 90 0 1
symversion=0.1
T 40800 48000 5 10 0 0 0 0 1
footprint=0603
T 40800 48000 5 10 1 1 0 0 1
value=10µ
}
C 42000 46100 1 180 0 resistor-2.sym
{
T 41600 45750 5 10 0 0 180 0 1
device=RESISTOR
T 41800 45800 5 10 1 1 180 0 1
refdes=R200
T 42000 46100 5 10 0 1 0 0 1
footprint=0805
}
C 41100 44400 1 0 0 inductor-1.sym
{
T 41300 44900 5 10 0 0 0 0 1
device=INDUCTOR
T 41300 44700 5 10 1 1 0 0 1
refdes=L200
T 41300 45100 5 10 0 0 0 0 1
symversion=0.1
T 41100 44400 5 10 1 1 0 0 1
value=MA5532
T 41100 44400 5 10 0 1 0 0 1
footprint=MA5532
}
N 41100 44500 41100 48000 4
N 42000 44500 42000 46500 4
N 42000 46500 41200 46500 4
N 41200 46500 41200 47700 4
N 41200 47700 41700 47700 4
N 41700 48000 41100 48000 4
N 41700 48900 40600 48900 4
N 40600 48000 40600 47400 4
N 40600 47400 41700 47400 4
{
T 40600 47400 5 10 1 1 0 0 1
netname=GND
}
N 41300 49500 42700 49500 4
N 44200 47700 44300 47700 4
N 45400 46800 45400 46500 4
{
T 45400 46800 5 10 1 1 0 0 1
netname=GND
}
N 41700 49200 41300 49200 4
N 41300 48300 41300 49500 4
N 41300 50400 44200 50400 4
{
T 41300 50400 5 10 1 1 0 0 1
netname=GND
}
N 44200 50400 44200 49500 4
N 44200 48600 44400 48600 4
N 44400 48600 44400 48300 4
{
T 44400 48600 5 10 1 1 0 0 1
netname=GND
}
N 44400 48300 44200 48300 4
N 41300 48300 41700 48300 4
N 41700 48600 41300 48600 4
N 44200 49200 45800 49200 4
{
T 44200 49200 5 10 1 1 0 0 1
netname=I2C_SCL
}
N 44200 48900 46000 48900 4
{
T 44200 48900 5 10 1 1 0 0 1
netname=I2C_SDA
}
N 42700 46200 45300 46200 4
{
T 42700 46200 5 10 1 1 0 0 1
netname=Vddsensor
}
N 42700 46200 42700 49500 4
N 44200 47400 44300 47400 4
N 44300 46700 44300 47400 4
N 44300 46700 45400 46700 4
N 44200 48000 42700 48000 4