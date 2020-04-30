EESchema Schematic File Version 4
EELAYER 26 0
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
L Connector:Conn_01x06_Male J1
U 1 1 5CFECE5A
P 4800 3250
F 0 "J1" H 4906 3628 50  0000 C CNN
F 1 "Conn_01x06_Male" H 4906 3537 50  0000 C CNN
F 2 "KiCadCustomLibs:Pads_prog_6pin1.25mm" H 4800 3250 50  0001 C CNN
F 3 "~" H 4800 3250 50  0001 C CNN
	1    4800 3250
	1    0    0    -1  
$EndComp
$Comp
L Connector_Generic:Conn_01x06 J2
U 1 1 5CFECEF5
P 5800 3250
F 0 "J2" H 5880 3242 50  0000 L CNN
F 1 "Conn_01x06" H 5880 3151 50  0000 L CNN
F 2 "KiCadCustomLibs:JST_SH_6x1mm" H 5800 3250 50  0001 C CNN
F 3 "~" H 5800 3250 50  0001 C CNN
	1    5800 3250
	1    0    0    -1  
$EndComp
$Comp
L Connector:Conn_01x06_Male J3
U 1 1 5CFED08B
P 4800 4000
F 0 "J3" H 4906 4378 50  0000 C CNN
F 1 "Conn_01x06_Male" H 4906 4287 50  0000 C CNN
F 2 "Pin_Headers:Pin_Header_Angled_1x06_Pitch1.27mm" H 4800 4000 50  0001 C CNN
F 3 "~" H 4800 4000 50  0001 C CNN
	1    4800 4000
	1    0    0    -1  
$EndComp
Wire Wire Line
	5000 3050 5100 3050
Wire Wire Line
	5600 3150 5200 3150
Wire Wire Line
	5000 3250 5300 3250
Wire Wire Line
	5600 3350 5400 3350
Wire Wire Line
	5000 3450 5500 3450
Wire Wire Line
	5600 3550 5000 3550
Wire Wire Line
	5000 4300 5600 4300
Wire Wire Line
	5600 4300 5600 3550
Connection ~ 5600 3550
Wire Wire Line
	5000 4200 5500 4200
Wire Wire Line
	5500 4200 5500 3450
Connection ~ 5500 3450
Wire Wire Line
	5500 3450 5600 3450
Wire Wire Line
	5000 4100 5400 4100
Wire Wire Line
	5400 4100 5400 3350
Connection ~ 5400 3350
Wire Wire Line
	5400 3350 5000 3350
Wire Wire Line
	5000 4000 5300 4000
Wire Wire Line
	5300 4000 5300 3250
Connection ~ 5300 3250
Wire Wire Line
	5300 3250 5600 3250
Wire Wire Line
	5000 3900 5200 3900
Wire Wire Line
	5200 3900 5200 3150
Connection ~ 5200 3150
Wire Wire Line
	5200 3150 5000 3150
Wire Wire Line
	5000 3800 5100 3800
Wire Wire Line
	5100 3800 5100 3050
Connection ~ 5100 3050
Wire Wire Line
	5100 3050 5600 3050
$EndSCHEMATC
