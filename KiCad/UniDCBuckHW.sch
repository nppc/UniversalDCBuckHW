EESchema Schematic File Version 2
LIBS:power
LIBS:device
LIBS:transistors
LIBS:conn
LIBS:linear
LIBS:regul
LIBS:74xx
LIBS:cmos4000
LIBS:adc-dac
LIBS:memory
LIBS:xilinx
LIBS:microcontrollers
LIBS:dsp
LIBS:microchip
LIBS:analog_switches
LIBS:motorola
LIBS:texas
LIBS:intel
LIBS:audio
LIBS:interface
LIBS:digital-audio
LIBS:philips
LIBS:display
LIBS:cypress
LIBS:siliconi
LIBS:opto
LIBS:atmel
LIBS:contrib
LIBS:valves
LIBS:UniDCBuckHW-cache
EELAYER 25 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 1 1
Title ""
Date ""
Rev ""
Comp ""
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
$Comp
L D_Schottky D1
U 1 1 5B646A06
P 5350 3100
F 0 "D1" H 5350 3200 50  0000 C CNN
F 1 "PPS1045" H 5350 3000 50  0000 C CNN
F 2 "Diodes_SMD:D_Powermite3" H 5350 3100 50  0001 C CNN
F 3 "" H 5350 3100 50  0001 C CNN
	1    5350 3100
	1    0    0    -1  
$EndComp
$Comp
L L_Core_Iron L1
U 1 1 5B646BB5
P 5900 3100
F 0 "L1" V 5850 3100 50  0000 C CNN
F 1 "47uH" V 6010 3100 50  0000 C CNN
F 2 "KiCadCustomLibs:PCV-2-473-10L" H 5900 3100 50  0001 C CNN
F 3 "" H 5900 3100 50  0001 C CNN
	1    5900 3100
	0    -1   -1   0   
$EndComp
$Comp
L Q_NMOS_GDS Q1
U 1 1 5B646E50
P 4000 3200
F 0 "Q1" H 4200 3250 50  0000 L CNN
F 1 "Q_NMOS_GDS" H 4200 3150 50  0000 L CNN
F 2 "TO_SOT_Packages_SMD:TO-252-2_Rectifier" H 4200 3300 50  0001 C CNN
F 3 "" H 4000 3200 50  0001 C CNN
	1    4000 3200
	0    -1   -1   0   
$EndComp
$EndSCHEMATC
