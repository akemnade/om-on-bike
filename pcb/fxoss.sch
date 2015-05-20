v 20110115 2
C 40000 40000 0 0 0 title-B.sym
C 44200 44100 1 0 0 fxos8700cq.sym
{
T 44200 47300 5 10 0 1 0 0 1
device=FXOS8700CQ
T 44195 44095 5 10 0 1 0 0 1
device=FXOS8700CQ
T 45015 44195 5 10 1 1 0 0 1
refdes=U300
T 44215 44095 5 10 0 1 0 0 1
footprint=FXOS8700CQ
}
N 46200 47500 46200 48700 4
C 45300 48600 1 0 0 resistor-2.sym
{
T 45700 48950 5 10 0 0 0 0 1
device=RESISTOR
T 45500 48900 5 10 1 1 0 0 1
refdes=R300
T 45300 48600 5 10 1 1 0 0 1
value=0
T 45300 48600 5 10 0 1 0 0 1
footprint=0603
}
N 45300 48700 44800 48700 4
{
T 44500 48500 5 10 1 1 0 0 1
netname=Vddsensor
}
N 44800 46600 44800 48200 4
N 44800 48200 46200 48200 4
C 44200 45400 1 90 0 capacitor-1.sym
{
T 43500 45600 5 10 0 0 90 0 1
device=CAPACITOR
T 43700 45600 5 10 1 1 90 0 1
refdes=C301
T 43300 45600 5 10 0 0 90 0 1
symversion=0.1
T 44200 45400 5 10 0 0 90 0 1
footprint=0603
T 44200 45400 5 10 1 1 90 0 1
value=100n
}
C 43700 45400 1 90 0 capacitor-1.sym
{
T 43000 45600 5 10 0 0 90 0 1
device=CAPACITOR
T 43200 45600 5 10 1 1 90 0 1
refdes=C300
T 42800 45600 5 10 0 0 90 0 1
symversion=0.1
T 43700 45400 5 10 0 0 90 0 1
footprint=0603
T 43700 45400 5 10 1 1 90 0 1
value=100n
}
N 44800 46600 43500 46600 4
N 43500 46600 43500 46300 4
N 44000 46300 44800 46300 4
N 43500 45400 44800 45400 4
{
T 43800 45400 5 10 1 1 0 0 1
netname=GND
}
C 46400 43100 1 90 0 capacitor-1.sym
{
T 45700 43300 5 10 0 0 90 0 1
device=CAPACITOR
T 45900 43300 5 10 1 1 90 0 1
refdes=C302
T 45500 43300 5 10 0 0 90 0 1
symversion=0.1
T 46400 43100 5 10 0 0 90 0 1
footprint=0603
T 46400 43100 5 10 1 1 90 0 1
value=100n
}
N 46200 44100 46200 44000 4
N 45600 47500 45600 48000 4
{
T 45600 47500 5 10 0 1 0 0 1
netname=GND
}
N 44800 46000 44300 46000 4
N 44300 46000 44300 45400 4
N 47300 46600 47800 46600 4
N 47800 43100 47800 46600 4
{
T 47800 46600 5 10 1 1 0 0 1
netname=GND
}
N 47800 46300 47300 46300 4
N 47300 45700 47800 45700 4
N 45900 44100 46000 44100 4
N 46000 44100 46000 47700 4
N 46000 47700 46200 47700 4
N 46200 43100 47800 43100 4
C 44700 43700 1 0 0 resistor-2.sym
{
T 45100 44050 5 10 0 0 0 0 1
device=RESISTOR
T 44900 44000 5 10 1 1 0 0 1
refdes=R302
T 44700 43700 5 10 1 1 0 0 1
value=0
T 44700 43700 5 10 0 1 0 0 1
footprint=0603
}
C 43700 44800 1 0 0 resistor-2.sym
{
T 44100 45150 5 10 0 0 0 0 1
device=RESISTOR
T 43900 45100 5 10 1 1 0 0 1
refdes=R301
T 43700 44800 5 10 1 1 0 0 1
value=0
T 43700 44800 5 10 0 1 0 0 1
footprint=0603
}
N 45600 44100 45600 43800 4
N 44800 45700 44600 45700 4
N 44600 45700 44600 44900 4
N 43700 44900 43100 44900 4
{
T 43100 45000 5 10 1 1 0 0 1
netname=I2C_SCL
}
N 44700 43800 44300 43800 4
{
T 44100 44000 5 10 1 1 0 0 1
netname=I2C_SDA
}
