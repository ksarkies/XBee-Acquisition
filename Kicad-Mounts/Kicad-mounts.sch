EESchema Schematic File Version 2
LIBS:DataStorage
LIBS:KB1LQC
LIBS:MiscellaneousDevices
LIBS:RF_OEM_Parts
LIBS:Sensors
LIBS:Kicad-mounts-cache
EELAYER 25 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 1 1
Title "Watermeter XBee and LED Mounting Boards"
Date "14 September 2013"
Rev "2.00"
Comp "Xerofill"
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
$Comp
L CONN_01X03 P6
U 1 1 53F679DA
P 6800 5700
F 0 "P6" V 6750 5700 50  0000 C CNN
F 1 "CONN_3" V 6850 5700 40  0000 C CNN
F 2 "Photodiode:SIL-3" H 6800 5700 60  0001 C CNN
F 3 "" H 6800 5700 60  0000 C CNN
	1    6800 5700
	1    0    0    -1  
$EndComp
Text Label 3450 4900 2    60   ~ 0
Bootloader
Text Label 3950 4900 0    60   ~ 0
VBatt
Text Label 3450 5000 2    60   ~ 0
Reset
Text Label 3950 5000 0    60   ~ 0
XBee-Reset
Text Label 3450 5100 2    60   ~ 0
Rx
Text Label 3950 5100 0    60   ~ 0
Sleep_Rq
Text Label 3450 5200 2    60   ~ 0
On/Sleep
Text Label 3950 5300 0    60   ~ 0
Tx
$Comp
L GND #PWR01
U 1 1 53F68519
P 3450 5300
F 0 "#PWR01" H 3450 5300 30  0001 C CNN
F 1 "GND" H 3450 5230 30  0001 C CNN
F 2 "" H 3450 5300 60  0000 C CNN
F 3 "" H 3450 5300 60  0000 C CNN
	1    3450 5300
	0    1    1    0   
$EndComp
$Comp
L VCC #PWR02
U 1 1 53F6854B
P 3950 4800
F 0 "#PWR02" H 3950 4900 30  0001 C CNN
F 1 "VCC" V 3950 4950 30  0000 C CNN
F 2 "" H 3950 4800 60  0000 C CNN
F 3 "" H 3950 4800 60  0000 C CNN
	1    3950 4800
	0    1    1    0   
$EndComp
Text Label 3000 3800 2    60   ~ 0
XBee-Reset
Text Label 3000 4200 2    60   ~ 0
Sleep_Rq
$Comp
L GND #PWR03
U 1 1 53F689D6
P 3000 4300
F 0 "#PWR03" H 3000 4300 30  0001 C CNN
F 1 "GND" H 3000 4230 30  0001 C CNN
F 2 "" H 3000 4300 60  0000 C CNN
F 3 "" H 3000 4300 60  0000 C CNN
	1    3000 4300
	1    0    0    -1  
$EndComp
Text Label 5100 4100 0    60   ~ 0
On/Sleep
Text Label 3000 3600 2    60   ~ 0
Tx
Text Label 3000 3500 2    60   ~ 0
Rx
Text Label 3450 4800 2    60   ~ 0
Commissioning
Text Label 5100 3800 0    60   ~ 0
RTS
Text Label 5100 4200 0    60   ~ 0
CTS
Text Label 3000 3700 2    60   ~ 0
Reset
Text Label 3000 4000 2    60   ~ 0
Bootloader
Text Label 5100 3500 0    60   ~ 0
VBatt
$Comp
L PWR_FLAG #FLG04
U 1 1 53F68599
P 3950 4800
F 0 "#FLG04" H 3950 4895 30  0001 C CNN
F 1 "PWR_FLAG" H 3950 4980 30  0000 C CNN
F 2 "" H 3950 4800 60  0000 C CNN
F 3 "" H 3950 4800 60  0000 C CNN
	1    3950 4800
	1    0    0    -1  
$EndComp
$Comp
L PWR_FLAG #FLG05
U 1 1 53F685B5
P 3450 5300
F 0 "#FLG05" H 3450 5395 30  0001 C CNN
F 1 "PWR_FLAG" H 3450 5480 30  0000 C CNN
F 2 "" H 3450 5300 60  0000 C CNN
F 3 "" H 3450 5300 60  0000 C CNN
	1    3450 5300
	-1   0    0    1   
$EndComp
Text Label 3950 5200 0    60   ~ 0
Association
$Comp
L TPS77033 U2
U 1 1 549A2F0F
P 3700 6100
F 0 "U2" H 3850 5704 60  0000 C CNN
F 1 "TPS77033" H 3700 6300 60  0000 C CNN
F 2 "Photodiode:SOT-23-5" H 3700 6100 60  0001 C CNN
F 3 "" H 3700 6100 60  0000 C CNN
	1    3700 6100
	1    0    0    -1  
$EndComp
$Comp
L C C1
U 1 1 549A30D6
P 2800 6200
F 0 "C1" H 2800 6300 40  0000 L CNN
F 1 "0.47uF" H 2806 6115 40  0000 L CNN
F 2 "Photodiode:SMD-0603_c" H 2838 6050 30  0001 C CNN
F 3 "" H 2800 6200 60  0000 C CNN
	1    2800 6200
	1    0    0    -1  
$EndComp
$Comp
L C C2
U 1 1 549A3151
P 4600 6350
F 0 "C2" H 4600 6450 40  0000 L CNN
F 1 "0.1uF" H 4606 6265 40  0000 L CNN
F 2 "Photodiode:SMD-0603_c" H 4638 6200 30  0001 C CNN
F 3 "" H 4600 6350 60  0000 C CNN
	1    4600 6350
	1    0    0    -1  
$EndComp
$Comp
L CP C3
U 1 1 549A31E1
P 5000 6350
F 0 "C3" H 5050 6450 40  0000 L CNN
F 1 "10uF" H 5050 6250 40  0000 L CNN
F 2 "Photodiode:SMD-1206_Pol" H 5100 6200 30  0001 C CNN
F 3 "" H 5000 6350 300 0000 C CNN
	1    5000 6350
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR06
U 1 1 549A3CE1
P 3700 6550
F 0 "#PWR06" H 3700 6550 30  0001 C CNN
F 1 "GND" H 3700 6480 30  0001 C CNN
F 2 "" H 3700 6550 60  0000 C CNN
F 3 "" H 3700 6550 60  0000 C CNN
	1    3700 6550
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR07
U 1 1 549A3D0B
P 4600 6500
F 0 "#PWR07" H 4600 6500 30  0001 C CNN
F 1 "GND" H 4600 6430 30  0001 C CNN
F 2 "" H 4600 6500 60  0000 C CNN
F 3 "" H 4600 6500 60  0000 C CNN
	1    4600 6500
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR08
U 1 1 549A3D2E
P 5000 6500
F 0 "#PWR08" H 5000 6500 30  0001 C CNN
F 1 "GND" H 5000 6430 30  0001 C CNN
F 2 "" H 5000 6500 60  0000 C CNN
F 3 "" H 5000 6500 60  0000 C CNN
	1    5000 6500
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR09
U 1 1 549A3D51
P 2800 6350
F 0 "#PWR09" H 2800 6350 30  0001 C CNN
F 1 "GND" H 2800 6280 30  0001 C CNN
F 2 "" H 2800 6350 60  0000 C CNN
F 3 "" H 2800 6350 60  0000 C CNN
	1    2800 6350
	1    0    0    -1  
$EndComp
NoConn ~ 4000 6050
$Comp
L GND #PWR010
U 1 1 549A3D98
P 3400 6300
F 0 "#PWR010" H 3400 6300 30  0001 C CNN
F 1 "GND" H 3400 6230 30  0001 C CNN
F 2 "" H 3400 6300 60  0000 C CNN
F 3 "" H 3400 6300 60  0000 C CNN
	1    3400 6300
	0    1    1    0   
$EndComp
$Comp
L VCC #PWR011
U 1 1 549A5134
P 2650 6050
F 0 "#PWR011" H 2650 6150 30  0001 C CNN
F 1 "VCC" V 2650 6200 30  0000 C CNN
F 2 "" H 2650 6050 60  0000 C CNN
F 3 "" H 2650 6050 60  0000 C CNN
	1    2650 6050
	0    -1   1    0   
$EndComp
Text Label 3000 3400 2    60   ~ 0
V3
Text Label 5200 6200 0    60   ~ 0
V3
Text Label 5100 3900 0    60   ~ 0
Association
Wire Wire Line
	6600 5600 6600 5500
Wire Wire Line
	6600 5500 5900 5500
Wire Wire Line
	6600 5700 6600 5800
Wire Wire Line
	6600 5800 6600 5800
Wire Wire Line
	6600 5800 5900 5800
Wire Wire Line
	2200 3400 3000 3400
Wire Wire Line
	2200 3500 3000 3500
Wire Wire Line
	2200 3600 3000 3600
Wire Wire Line
	2200 3700 3000 3700
Wire Wire Line
	2200 3800 3000 3800
Wire Wire Line
	2200 3900 3000 3900
Wire Wire Line
	2200 4000 3000 4000
Wire Wire Line
	2200 4100 3000 4100
Wire Wire Line
	2200 4200 3000 4200
Wire Wire Line
	2200 4300 3000 4300
Wire Wire Line
	5100 3400 6000 3400
Wire Wire Line
	5100 3500 6000 3500
Wire Wire Line
	5100 3600 6000 3600
Wire Wire Line
	5100 3700 6000 3700
Wire Wire Line
	5100 3800 6000 3800
Wire Wire Line
	5100 3900 6000 3900
Wire Wire Line
	5100 4000 6000 4000
Wire Wire Line
	5100 4100 6000 4100
Wire Wire Line
	5100 4200 6000 4200
Wire Wire Line
	5100 4300 6000 4300
Connection ~ 6600 5800
Wire Wire Line
	2650 6050 2800 6050
Wire Wire Line
	2800 6050 3400 6050
Wire Wire Line
	4000 6200 4600 6200
Wire Wire Line
	4600 6200 5000 6200
Wire Wire Line
	5000 6200 5200 6200
Connection ~ 5000 6200
Connection ~ 4600 6200
Connection ~ 2800 6050
$Comp
L CONN_02X06 P3
U 1 1 53F67592
P 3700 5050
F 0 "P3" H 3700 5400 60  0000 C CNN
F 1 "CONN_6X2" V 3700 5050 60  0000 C CNN
F 2 "Photodiode:pin_header_2mm_2x6" H 3700 5050 60  0001 C CNN
F 3 "" H 3700 5050 60  0000 C CNN
	1    3700 5050
	1    0    0    -1  
$EndComp
Text Label 5100 3400 0    60   ~ 0
Commissioning
$Comp
L LED_ALT D1
U 1 1 5887D976
P 5900 5650
F 0 "D1" V 5900 5800 50  0000 C CNN
F 1 "SFH4056" V 6150 5650 50  0000 C CNN
F 2 "" H 5900 5650 50  0000 C CNN
F 3 "" H 5900 5650 50  0000 C CNN
	1    5900 5650
	0    -1   -1   0   
$EndComp
$Comp
L XBEE-ZB XB?
U 1 1 58B79FBE
P 4050 3850
F 0 "XB?" H 4050 4650 60  0000 C CNN
F 1 "XBEE-ZB" H 4050 4750 60  0000 C CNN
F 2 "" H 4050 3850 60  0000 C CNN
F 3 "" H 4050 3850 60  0000 C CNN
	1    4050 3850
	1    0    0    -1  
$EndComp
$EndSCHEMATC
