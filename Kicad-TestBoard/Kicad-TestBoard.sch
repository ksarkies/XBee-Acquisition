EESchema Schematic File Version 2
LIBS:74xgxx
LIBS:74xx
LIBS:ac-dc
LIBS:actel
LIBS:adc-dac
LIBS:allegro
LIBS:Altera
LIBS:analog_devices
LIBS:analog_switches
LIBS:atmel
LIBS:audio
LIBS:battery_management
LIBS:bbd
LIBS:brooktre
LIBS:cmos4000
LIBS:cmos_ieee
LIBS:conn
LIBS:contrib
LIBS:cypress
LIBS:dc-dc
LIBS:device
LIBS:digital-audio
LIBS:diode
LIBS:display
LIBS:dsp
LIBS:elec-unifil
LIBS:ESD_Protection
LIBS:ftdi
LIBS:gennum
LIBS:graphic
LIBS:hc11
LIBS:intel
LIBS:interface
LIBS:ir
LIBS:Lattice
LIBS:linear
LIBS:logo
LIBS:maxim
LIBS:mechanical
LIBS:memory
LIBS:microchip
LIBS:microchip_dspic33dsc
LIBS:microchip_pic10mcu
LIBS:microchip_pic12mcu
LIBS:microchip_pic16mcu
LIBS:microchip_pic18mcu
LIBS:microchip_pic32mcu
LIBS:microcontrollers
LIBS:motor_drivers
LIBS:motorola
LIBS:motors
LIBS:msp430
LIBS:nordicsemi
LIBS:nxp_armmcu
LIBS:onsemi
LIBS:opto
LIBS:Oscillators
LIBS:philips
LIBS:power
LIBS:powerint
LIBS:Power_Management
LIBS:pspice
LIBS:references
LIBS:regul
LIBS:relays
LIBS:rfcom
LIBS:sensors
LIBS:silabs
LIBS:siliconi
LIBS:stm8
LIBS:stm32
LIBS:supertex
LIBS:switches
LIBS:texas
LIBS:transf
LIBS:transistors
LIBS:ttl_ieee
LIBS:valves
LIBS:video
LIBS:wiznet
LIBS:Worldsemi
LIBS:Xicor
LIBS:xilinx
LIBS:zetex
LIBS:Zilog
LIBS:Kicad-TestBoard-cache
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
L CONN_02X06 P2
U 1 1 57576B01
P 2600 3550
F 0 "P2" H 2600 4015 50  0000 C CNN
F 1 "Debug" H 2600 3924 50  0000 C CNN
F 2 "Testboard:pin_header_2mm_2x6_shrouded" H 2600 2350 50  0001 C CNN
F 3 "" H 2600 2350 50  0000 C CNN
	1    2600 3550
	1    0    0    -1  
$EndComp
$Comp
L MAX232 U2
U 1 1 57576C0D
P 6350 4200
F 0 "U2" H 6000 5100 50  0000 C CNN
F 1 "MAX232" H 6350 4850 50  0000 C CNN
F 2 "Testboard:DIP-16_W7.62mm" H 6350 4300 50  0001 C CNN
F 3 "" H 6350 4300 50  0000 C CNN
	1    6350 4200
	1    0    0    -1  
$EndComp
$Comp
L DB9 J1
U 1 1 57576C59
P 9200 4700
F 0 "J1" H 9450 4550 50  0000 C CNN
F 1 "DB9 RS-232 Female" H 9150 5350 50  0000 C CNN
F 2 "Testboard:DB9FD" H 9200 4700 50  0001 C CNN
F 3 "" H 9200 4700 50  0000 C CNN
	1    9200 4700
	1    0    0    1   
$EndComp
$Comp
L CONN_01X02 P1
U 1 1 57576CD9
P 1800 1900
F 0 "P1" H 1878 1941 50  0000 L CNN
F 1 "Power 6V-12V" H 1878 1850 50  0000 L CNN
F 2 "Testboard:PINHEAD1-2" H 1800 1900 50  0001 C CNN
F 3 "" H 1800 1900 50  0000 C CNN
	1    1800 1900
	-1   0    0    -1  
$EndComp
$Comp
L CONN_01X04 P5
U 1 1 57576D2B
P 9150 3700
F 0 "P5" H 9350 3600 50  0000 C CNN
F 1 "RS-232" H 9300 4000 50  0000 C CNN
F 2 "Testboard:PINHEAD1-4" H 9150 3700 50  0001 C CNN
F 3 "" H 9150 3700 50  0000 C CNN
	1    9150 3700
	1    0    0    1   
$EndComp
$Comp
L CONN_01X06 P4
U 1 1 57576DE6
P 4350 4000
F 0 "P4" H 4428 4041 50  0000 L CNN
F 1 "AVR Programmer" H 4428 3950 50  0000 L CNN
F 2 "Testboard:PINHEAD1-6" H 4350 4000 50  0001 C CNN
F 3 "" H 4350 4000 50  0000 C CNN
	1    4350 4000
	1    0    0    -1  
$EndComp
$Comp
L 7805 U1
U 1 1 57576E80
P 3400 2400
F 0 "U1" H 3400 2715 50  0000 C CNN
F 1 "78L05" H 3400 2624 50  0000 C CNN
F 2 "Testboard:TO-92_78L05" H 3400 2400 50  0001 C CNN
F 3 "" H 3400 2400 50  0000 C CNN
	1    3400 2400
	1    0    0    -1  
$EndComp
Text Label 2350 3300 2    60   ~ 0
Battery
$Comp
L Earth #PWR01
U 1 1 575799DF
P 3400 2650
F 0 "#PWR01" H 3400 2400 50  0001 C CNN
F 1 "Earth" H 3400 2500 50  0001 C CNN
F 2 "" H 3400 2650 50  0000 C CNN
F 3 "" H 3400 2650 50  0000 C CNN
	1    3400 2650
	1    0    0    -1  
$EndComp
$Comp
L Earth #PWR02
U 1 1 57579AC2
P 2000 1950
F 0 "#PWR02" H 2000 1700 50  0001 C CNN
F 1 "Earth" H 2000 1800 50  0001 C CNN
F 2 "" H 2000 1950 50  0000 C CNN
F 3 "" H 2000 1950 50  0000 C CNN
	1    2000 1950
	1    0    0    -1  
$EndComp
Text Label 2350 3400 2    60   ~ 0
Reset
Text Label 2350 3500 2    60   ~ 0
SCK
Text Label 2350 3600 2    60   ~ 0
Tx
Text Label 2850 3300 0    60   ~ 0
Count
Text Label 2850 3400 0    60   ~ 0
MOSI
Text Label 2850 3500 0    60   ~ 0
MISO
Text Label 2850 3600 0    60   ~ 0
Rx
Text Label 2850 3700 0    60   ~ 0
On_Sleep
Text Label 2850 3800 0    60   ~ 0
Commissioning
$Comp
L SW_PUSH SW1
U 1 1 5757B167
P 1300 3400
F 0 "SW1" H 1300 3655 50  0000 C CNN
F 1 "Reset" H 1300 3564 50  0000 C CNN
F 2 "Testboard:SW_PUSH_TINY" H 1300 3400 50  0001 C CNN
F 3 "" H 1300 3400 50  0000 C CNN
	1    1300 3400
	1    0    0    -1  
$EndComp
$Comp
L Earth #PWR03
U 1 1 5757B92E
P 1000 3400
F 0 "#PWR03" H 1000 3150 50  0001 C CNN
F 1 "Earth" H 1000 3250 50  0001 C CNN
F 2 "" H 1000 3400 50  0000 C CNN
F 3 "" H 1000 3400 50  0000 C CNN
	1    1000 3400
	1    0    0    -1  
$EndComp
$Comp
L SW_PUSH SW2
U 1 1 5757BABE
P 1300 3950
F 0 "SW2" H 1300 4205 50  0000 C CNN
F 1 "Commissioning" H 1300 4114 50  0000 C CNN
F 2 "Testboard:SW_PUSH_TINY" H 1300 3950 50  0001 C CNN
F 3 "" H 1300 3950 50  0000 C CNN
	1    1300 3950
	1    0    0    -1  
$EndComp
$Comp
L Earth #PWR04
U 1 1 5757BB82
P 1000 3950
F 0 "#PWR04" H 1000 3700 50  0001 C CNN
F 1 "Earth" H 1000 3800 50  0001 C CNN
F 2 "" H 1000 3950 50  0000 C CNN
F 3 "" H 1000 3950 50  0000 C CNN
	1    1000 3950
	1    0    0    -1  
$EndComp
$Comp
L TEST_1P W1
U 1 1 5757BDA4
P 3350 3300
F 0 "W1" H 3408 3420 50  0000 L CNN
F 1 "TEST_1P" H 3408 3329 50  0000 L CNN
F 2 "Testboard:SIL-1" H 3550 3300 50  0001 C CNN
F 3 "" H 3550 3300 50  0000 C CNN
	1    3350 3300
	1    0    0    -1  
$EndComp
$Comp
L TEST_1P W2
U 1 1 5757BE94
P 4600 2450
F 0 "W2" H 4658 2570 50  0000 L CNN
F 1 "TEST_1P" H 4658 2479 50  0000 L CNN
F 2 "Testboard:SIL-1" H 4800 2450 50  0001 C CNN
F 3 "" H 4800 2450 50  0000 C CNN
	1    4600 2450
	1    0    0    -1  
$EndComp
$Comp
L Earth #PWR05
U 1 1 5757BFBE
P 4600 2450
F 0 "#PWR05" H 4600 2200 50  0001 C CNN
F 1 "Earth" H 4600 2300 50  0001 C CNN
F 2 "" H 4600 2450 50  0000 C CNN
F 3 "" H 4600 2450 50  0000 C CNN
	1    4600 2450
	1    0    0    -1  
$EndComp
$Comp
L Earth #PWR06
U 1 1 5757C491
P 4150 3500
F 0 "#PWR06" H 4150 3250 50  0001 C CNN
F 1 "Earth" H 4150 3350 50  0001 C CNN
F 2 "" H 4150 3500 50  0000 C CNN
F 3 "" H 4150 3500 50  0000 C CNN
	1    4150 3500
	1    0    0    -1  
$EndComp
$Comp
L CONN_01X03 P3
U 1 1 5757C5D7
P 4350 3400
F 0 "P3" H 4427 3441 50  0000 L CNN
F 1 "Arduino" H 4427 3350 50  0000 L CNN
F 2 "Testboard:PINHEAD1-3" H 4350 3400 50  0001 C CNN
F 3 "" H 4350 3400 50  0000 C CNN
	1    4350 3400
	1    0    0    1   
$EndComp
$Comp
L Earth #PWR07
U 1 1 5757CADB
P 4150 3750
F 0 "#PWR07" H 4150 3500 50  0001 C CNN
F 1 "Earth" H 4150 3600 50  0001 C CNN
F 2 "" H 4150 3750 50  0000 C CNN
F 3 "" H 4150 3750 50  0000 C CNN
	1    4150 3750
	1    0    0    1   
$EndComp
$Comp
L Earth #PWR08
U 1 1 5757E113
P 8950 3550
F 0 "#PWR08" H 8950 3300 50  0001 C CNN
F 1 "Earth" H 8950 3400 50  0001 C CNN
F 2 "" H 8950 3550 50  0000 C CNN
F 3 "" H 8950 3550 50  0000 C CNN
	1    8950 3550
	1    0    0    1   
$EndComp
$Comp
L Q_PNP_EBC Q3
U 1 1 5757E5D8
P 3750 6150
F 0 "Q3" H 3941 6196 50  0000 L CNN
F 1 "Commissioning" H 3941 6105 50  0000 L CNN
F 2 "Testboard:TO-92_Molded_Narrow" H 3950 6250 50  0001 C CNN
F 3 "" H 3750 6150 50  0000 C CNN
	1    3750 6150
	1    0    0    -1  
$EndComp
$Comp
L R R6
U 1 1 5757E668
P 3850 5800
F 0 "R6" H 3920 5846 50  0000 L CNN
F 1 "470" H 3920 5755 50  0000 L CNN
F 2 "Testboard:Resistor_Horizontal_RM10mm" V 3780 5800 50  0001 C CNN
F 3 "" H 3850 5800 50  0000 C CNN
	1    3850 5800
	1    0    0    -1  
$EndComp
$Comp
L VCC #PWR09
U 1 1 5757E882
P 3800 2350
F 0 "#PWR09" H 3800 2200 50  0001 C CNN
F 1 "VCC" H 3817 2523 50  0000 C CNN
F 2 "" H 3800 2350 50  0000 C CNN
F 3 "" H 3800 2350 50  0000 C CNN
	1    3800 2350
	1    0    0    -1  
$EndComp
$Comp
L VCC #PWR010
U 1 1 5757E8CC
P 6350 3000
F 0 "#PWR010" H 6350 2850 50  0001 C CNN
F 1 "VCC" H 6367 3173 50  0000 C CNN
F 2 "" H 6350 3000 50  0000 C CNN
F 3 "" H 6350 3000 50  0000 C CNN
	1    6350 3000
	1    0    0    -1  
$EndComp
$Comp
L Earth #PWR011
U 1 1 5757EBCE
P 3850 6350
F 0 "#PWR011" H 3850 6100 50  0001 C CNN
F 1 "Earth" H 3850 6200 50  0001 C CNN
F 2 "" H 3850 6350 50  0000 C CNN
F 3 "" H 3850 6350 50  0000 C CNN
	1    3850 6350
	1    0    0    -1  
$EndComp
$Comp
L VCC #PWR012
U 1 1 5757EC33
P 3850 5350
F 0 "#PWR012" H 3850 5200 50  0001 C CNN
F 1 "VCC" H 3867 5523 50  0000 C CNN
F 2 "" H 3850 5350 50  0000 C CNN
F 3 "" H 3850 5350 50  0000 C CNN
	1    3850 5350
	1    0    0    -1  
$EndComp
$Comp
L R R5
U 1 1 5757F1DE
P 3550 6000
F 0 "R5" H 3500 5800 50  0000 R CNN
F 1 "10K" H 3500 5900 50  0000 R CNN
F 2 "Testboard:Resistor_Horizontal_RM10mm" V 3480 6000 50  0001 C CNN
F 3 "" H 3550 6000 50  0000 C CNN
	1    3550 6000
	-1   0    0    1   
$EndComp
$Comp
L R R3
U 1 1 57581206
P 2800 5800
F 0 "R3" H 2870 5846 50  0000 L CNN
F 1 "470" H 2870 5755 50  0000 L CNN
F 2 "Testboard:Resistor_Horizontal_RM10mm" V 2730 5800 50  0001 C CNN
F 3 "" H 2800 5800 50  0000 C CNN
	1    2800 5800
	-1   0    0    -1  
$EndComp
$Comp
L VCC #PWR013
U 1 1 5758127D
P 2800 5350
F 0 "#PWR013" H 2800 5200 50  0001 C CNN
F 1 "VCC" H 2817 5523 50  0000 C CNN
F 2 "" H 2800 5350 50  0000 C CNN
F 3 "" H 2800 5350 50  0000 C CNN
	1    2800 5350
	-1   0    0    -1  
$EndComp
$Comp
L Q_PNP_EBC Q2
U 1 1 5758134F
P 2900 6150
F 0 "Q2" H 3091 6196 50  0000 L CNN
F 1 "On_Sleep" H 3091 6105 50  0000 L CNN
F 2 "Testboard:TO-92_Molded_Narrow" H 3100 6250 50  0001 C CNN
F 3 "" H 2900 6150 50  0000 C CNN
	1    2900 6150
	-1   0    0    -1  
$EndComp
$Comp
L R R4
U 1 1 575813DD
P 3100 6000
F 0 "R4" H 3050 5800 50  0000 R CNN
F 1 "10K" H 3050 5900 50  0000 R CNN
F 2 "Testboard:Resistor_Horizontal_RM10mm" V 3030 6000 50  0001 C CNN
F 3 "" H 3100 6000 50  0000 C CNN
	1    3100 6000
	1    0    0    1   
$EndComp
$Comp
L Earth #PWR014
U 1 1 5758197B
P 2800 6350
F 0 "#PWR014" H 2800 6100 50  0001 C CNN
F 1 "Earth" H 2800 6200 50  0001 C CNN
F 2 "" H 2800 6350 50  0000 C CNN
F 3 "" H 2800 6350 50  0000 C CNN
	1    2800 6350
	-1   0    0    -1  
$EndComp
$Comp
L VCC #PWR015
U 1 1 57582756
P 1650 5350
F 0 "#PWR015" H 1650 5200 50  0001 C CNN
F 1 "VCC" H 1667 5523 50  0000 C CNN
F 2 "" H 1650 5350 50  0000 C CNN
F 3 "" H 1650 5350 50  0000 C CNN
	1    1650 5350
	1    0    0    -1  
$EndComp
$Comp
L R R1
U 1 1 575827A9
P 1650 5800
F 0 "R1" H 1720 5846 50  0000 L CNN
F 1 "470" H 1720 5755 50  0000 L CNN
F 2 "Testboard:Resistor_Horizontal_RM10mm" V 1580 5800 50  0001 C CNN
F 3 "" H 1650 5800 50  0000 C CNN
	1    1650 5800
	1    0    0    -1  
$EndComp
$Comp
L Q_PNP_EBC Q1
U 1 1 575828F8
P 1750 6150
F 0 "Q1" H 1941 6196 50  0000 L CNN
F 1 "Association" H 1941 6105 50  0000 L CNN
F 2 "Testboard:TO-92_Molded_Narrow" H 1950 6250 50  0001 C CNN
F 3 "" H 1750 6150 50  0000 C CNN
	1    1750 6150
	-1   0    0    -1  
$EndComp
$Comp
L R R2
U 1 1 57582990
P 1950 6000
F 0 "R2" H 2150 5850 50  0000 R CNN
F 1 "10K" H 2150 5950 50  0000 R CNN
F 2 "Testboard:Resistor_Horizontal_RM10mm" V 1880 6000 50  0001 C CNN
F 3 "" H 1950 6000 50  0000 C CNN
	1    1950 6000
	1    0    0    1   
$EndComp
$Comp
L Earth #PWR016
U 1 1 57582A1D
P 1650 6350
F 0 "#PWR016" H 1650 6100 50  0001 C CNN
F 1 "Earth" H 1650 6200 50  0001 C CNN
F 2 "" H 1650 6350 50  0000 C CNN
F 3 "" H 1650 6350 50  0000 C CNN
	1    1650 6350
	-1   0    0    -1  
$EndComp
$Comp
L CP C3
U 1 1 575837C6
P 5550 3450
F 0 "C3" H 5300 3550 50  0000 L CNN
F 1 "1uF" H 5300 3450 50  0000 L CNN
F 2 "Testboard:C_Radial_D5_L11_P2.5" H 5588 3300 50  0001 C CNN
F 3 "" H 5550 3450 50  0000 C CNN
	1    5550 3450
	1    0    0    -1  
$EndComp
$Comp
L CP C4
U 1 1 5758391E
P 7150 3450
F 0 "C4" H 7350 3550 50  0000 L CNN
F 1 "1uF" H 7350 3400 50  0000 L CNN
F 2 "Testboard:C_Radial_D5_L11_P2.5" H 7188 3300 50  0001 C CNN
F 3 "" H 7150 3450 50  0000 C CNN
	1    7150 3450
	1    0    0    -1  
$EndComp
$Comp
L CP C5
U 1 1 575839CD
P 7300 3800
F 0 "C5" V 7450 3750 50  0000 C CNN
F 1 "1uF" V 7450 3600 50  0000 C CNN
F 2 "Testboard:C_Radial_D5_L11_P2.5" H 7338 3650 50  0001 C CNN
F 3 "" H 7300 3800 50  0000 C CNN
	1    7300 3800
	0    -1   -1   0   
$EndComp
$Comp
L CP C6
U 1 1 57583B53
P 7300 4100
F 0 "C6" V 7200 4250 50  0000 C CNN
F 1 "1uF" V 7200 4400 50  0000 C CNN
F 2 "Testboard:C_Radial_D5_L11_P2.5" H 7338 3950 50  0001 C CNN
F 3 "" H 7300 4100 50  0000 C CNN
	1    7300 4100
	0    1    -1   0   
$EndComp
$Comp
L Earth #PWR017
U 1 1 57583CC1
P 7450 4100
F 0 "#PWR017" H 7450 3850 50  0001 C CNN
F 1 "Earth" H 7450 3950 50  0001 C CNN
F 2 "" H 7450 4100 50  0000 C CNN
F 3 "" H 7450 4100 50  0000 C CNN
	1    7450 4100
	0    -1   1    0   
$EndComp
$Comp
L R R7
U 1 1 575840B2
P 4700 4850
F 0 "R7" H 4770 4896 50  0000 L CNN
F 1 "1K" H 4770 4805 50  0000 L CNN
F 2 "Testboard:Resistor_Horizontal_RM10mm" V 4630 4850 50  0001 C CNN
F 3 "" H 4700 4850 50  0000 C CNN
	1    4700 4850
	1    0    0    -1  
$EndComp
$Comp
L R R8
U 1 1 57584208
P 5000 4700
F 0 "R8" V 5100 4700 50  0000 C CNN
F 1 "470" V 4884 4700 50  0000 C CNN
F 2 "Testboard:Resistor_Horizontal_RM10mm" V 4930 4700 50  0001 C CNN
F 3 "" H 5000 4700 50  0000 C CNN
	1    5000 4700
	0    1    1    0   
$EndComp
$Comp
L Earth #PWR018
U 1 1 5758448E
P 4700 5000
F 0 "#PWR018" H 4700 4750 50  0001 C CNN
F 1 "Earth" H 4700 4850 50  0001 C CNN
F 2 "" H 4700 5000 50  0000 C CNN
F 3 "" H 4700 5000 50  0000 C CNN
	1    4700 5000
	1    0    0    -1  
$EndComp
$Comp
L C C1
U 1 1 57584A93
P 2550 2500
F 0 "C1" H 2350 2450 50  0000 L CNN
F 1 "0.1uF" H 2300 2350 50  0000 L CNN
F 2 "Testboard:C_Disc_D3_P2.5" H 2588 2350 50  0001 C CNN
F 3 "" H 2550 2500 50  0000 C CNN
	1    2550 2500
	1    0    0    -1  
$EndComp
$Comp
L Earth #PWR019
U 1 1 57584E0E
P 2550 2650
F 0 "#PWR019" H 2550 2400 50  0001 C CNN
F 1 "Earth" H 2550 2500 50  0001 C CNN
F 2 "" H 2550 2650 50  0000 C CNN
F 3 "" H 2550 2650 50  0000 C CNN
	1    2550 2650
	1    0    0    -1  
$EndComp
$Comp
L Earth #PWR020
U 1 1 57584E82
P 2850 2650
F 0 "#PWR020" H 2850 2400 50  0001 C CNN
F 1 "Earth" H 2850 2500 50  0001 C CNN
F 2 "" H 2850 2650 50  0000 C CNN
F 3 "" H 2850 2650 50  0000 C CNN
	1    2850 2650
	1    0    0    -1  
$EndComp
$Comp
L VCC #PWR021
U 1 1 575852C0
P 4150 4250
F 0 "#PWR021" H 4150 4100 50  0001 C CNN
F 1 "VCC" V 4168 4377 50  0000 L CNN
F 2 "" H 4150 4250 50  0000 C CNN
F 3 "" H 4150 4250 50  0000 C CNN
	1    4150 4250
	0    -1   -1   0   
$EndComp
$Comp
L Earth #PWR022
U 1 1 5758599D
P 7450 3800
F 0 "#PWR022" H 7450 3550 50  0001 C CNN
F 1 "Earth" H 7450 3650 50  0001 C CNN
F 2 "" H 7450 3800 50  0000 C CNN
F 3 "" H 7450 3800 50  0000 C CNN
	1    7450 3800
	0    -1   1    0   
$EndComp
$Comp
L Earth #PWR023
U 1 1 5758BB8A
P 8750 5100
F 0 "#PWR023" H 8750 4850 50  0001 C CNN
F 1 "Earth" H 8750 4950 50  0001 C CNN
F 2 "" H 8750 5100 50  0000 C CNN
F 3 "" H 8750 5100 50  0000 C CNN
	1    8750 5100
	0    1    1    0   
$EndComp
$Comp
L Earth #PWR024
U 1 1 57579A7B
P 2050 2550
F 0 "#PWR024" H 2050 2300 50  0001 C CNN
F 1 "Earth" H 2050 2400 50  0001 C CNN
F 2 "" H 2050 2550 50  0000 C CNN
F 3 "" H 2050 2550 50  0000 C CNN
	1    2050 2550
	1    0    0    -1  
$EndComp
$Comp
L BARREL_JACK CON1
U 1 1 575769E4
P 1750 2450
F 0 "CON1" H 1372 2427 50  0000 R CNN
F 1 "Power 6V-12V" H 1700 2700 50  0000 R CNN
F 2 "Testboard:BARREL_JACK" H 1750 2450 50  0001 C CNN
F 3 "" H 1750 2450 50  0000 C CNN
	1    1750 2450
	1    0    0    -1  
$EndComp
$Comp
L CP C2
U 1 1 57580417
P 2850 2500
F 0 "C2" H 2950 2450 50  0000 L CNN
F 1 "10uF" H 2900 2350 50  0000 L CNN
F 2 "Testboard:C_Radial_D5_L11_P2.5" H 2888 2350 50  0001 C CNN
F 3 "" H 2850 2500 50  0000 C CNN
	1    2850 2500
	1    0    0    -1  
$EndComp
NoConn ~ 8950 3850
NoConn ~ 8750 5000
NoConn ~ 7150 4300
NoConn ~ 7150 4900
NoConn ~ 5550 4300
NoConn ~ 5550 4900
NoConn ~ 4150 3400
$Comp
L Earth #PWR025
U 1 1 57635380
P 2350 3800
F 0 "#PWR025" H 2350 3550 50  0001 C CNN
F 1 "Earth" H 2350 3650 50  0001 C CNN
F 2 "" H 2350 3800 50  0000 C CNN
F 3 "" H 2350 3800 50  0000 C CNN
	1    2350 3800
	1    0    0    -1  
$EndComp
$Comp
L Earth #PWR026
U 1 1 57637FFA
P 6350 5400
F 0 "#PWR026" H 6350 5150 50  0001 C CNN
F 1 "Earth" H 6350 5250 50  0001 C CNN
F 2 "" H 6350 5400 50  0000 C CNN
F 3 "" H 6350 5400 50  0000 C CNN
	1    6350 5400
	1    0    0    -1  
$EndComp
Wire Wire Line
	2050 2350 3000 2350
Wire Wire Line
	2000 1850 2400 1850
Wire Wire Line
	2400 1850 2400 2350
Connection ~ 2400 2350
Wire Wire Line
	2350 3300 2250 3300
Wire Wire Line
	2250 3400 2250 4150
Wire Wire Line
	2350 3500 2150 3500
Wire Wire Line
	2150 3500 2150 4050
Wire Wire Line
	2350 3600 2050 3600
Wire Wire Line
	2050 3600 2050 4500
Wire Wire Line
	1950 3700 1950 5850
Wire Wire Line
	2850 3300 4150 3300
Wire Wire Line
	2850 3400 4000 3400
Wire Wire Line
	2850 3500 3900 3500
Wire Wire Line
	2850 3600 3200 3600
Wire Wire Line
	3200 3600 3200 4700
Wire Wire Line
	2850 3700 3100 3700
Wire Wire Line
	3100 3700 3100 5850
Wire Wire Line
	2850 3800 3300 3800
Wire Wire Line
	3300 3800 3300 3950
Wire Wire Line
	3550 3950 3550 5850
Connection ~ 2250 3400
Wire Wire Line
	1600 3400 2350 3400
Wire Wire Line
	1600 3950 3550 3950
Connection ~ 3350 3300
Wire Wire Line
	4000 3400 4000 3850
Wire Wire Line
	4000 3850 4150 3850
Wire Wire Line
	3900 3500 3900 3950
Wire Wire Line
	3900 3950 4150 3950
Wire Wire Line
	2150 4050 4150 4050
Wire Wire Line
	2250 4150 4150 4150
Wire Wire Line
	2050 4500 5550 4500
Connection ~ 3300 3950
Wire Wire Line
	3200 4700 4850 4700
Connection ~ 2850 2350
Connection ~ 2550 2350
Wire Wire Line
	8150 3750 8150 4700
Wire Wire Line
	8000 3650 8000 4500
Wire Wire Line
	7150 4500 8750 4500
Wire Wire Line
	8000 3650 8950 3650
Wire Wire Line
	8150 3750 8950 3750
Connection ~ 8000 4500
Wire Wire Line
	7150 4700 8750 4700
Connection ~ 8150 4700
Wire Wire Line
	8750 4300 8600 4300
Wire Wire Line
	8600 4300 8600 4900
Wire Wire Line
	8600 4400 8750 4400
Wire Wire Line
	8600 4900 8750 4900
Connection ~ 8600 4400
Wire Wire Line
	8750 4600 8700 4600
Wire Wire Line
	8700 4600 8700 4800
Wire Wire Line
	8700 4800 8750 4800
Connection ~ 2250 2350
Connection ~ 2050 2550
Connection ~ 4700 4700
Wire Wire Line
	1950 3700 2350 3700
Wire Wire Line
	2050 2450 2050 2550
Wire Wire Line
	2250 2350 2250 2750
Wire Wire Line
	2250 3300 2250 2950
Text Notes 4750 5400 0    60   ~ 0
Remove jumper\nif XBee present.\nRisk of Damage.
Text Notes 1000 3000 0    60   ~ 0
Remove Jumper\nif Battery present.\nRisk of fire.
$Comp
L Jumper_NO_Small JP2
U 1 1 588EF989
P 5350 4700
F 0 "JP2" H 5350 4780 50  0000 C CNN
F 1 "Tx Jumper" H 5360 4640 50  0000 C CNN
F 2 "Testboard:SIL-1" H 5350 4700 50  0001 C CNN
F 3 "" H 5350 4700 50  0000 C CNN
	1    5350 4700
	1    0    0    -1  
$EndComp
Wire Wire Line
	5250 4700 5150 4700
Wire Wire Line
	5550 4700 5450 4700
$Comp
L Jumper_NO_Small JP1
U 1 1 588F1CAC
P 2250 2850
F 0 "JP1" H 2250 2930 50  0000 C CNN
F 1 "Power Jumper" H 2260 2790 50  0000 C CNN
F 2 "Testboard:SIL-1" H 2250 2850 50  0001 C CNN
F 3 "" H 2250 2850 50  0000 C CNN
	1    2250 2850
	0    1    1    0   
$EndComp
$Comp
L LED_ALT D3
U 1 1 588F7D20
P 3850 5500
F 0 "D3" H 3850 5600 50  0000 C CNN
F 1 "Red LED" H 3850 5300 50  0000 C CNN
F 2 "Testboard:LED-5MM" H 3850 5500 50  0001 C CNN
F 3 "" H 3850 5500 50  0000 C CNN
	1    3850 5500
	0    -1   -1   0   
$EndComp
$Comp
L LED_ALT D2
U 1 1 588F9DC4
P 2800 5500
F 0 "D2" H 2800 5600 50  0000 C CNN
F 1 "Green LED" H 2800 5350 50  0000 C CNN
F 2 "Testboard:LED-5MM" H 2800 5500 50  0001 C CNN
F 3 "" H 2800 5500 50  0000 C CNN
	1    2800 5500
	0    -1   -1   0   
$EndComp
$Comp
L LED_ALT D1
U 1 1 588FAFA8
P 1650 5500
F 0 "D1" H 1650 5600 50  0000 C CNN
F 1 "Yellow LED" H 1700 5350 50  0000 C CNN
F 2 "Testboard:LED-5MM" H 1650 5500 50  0001 C CNN
F 3 "" H 1650 5500 50  0000 C CNN
	1    1650 5500
	0    -1   -1   0   
$EndComp
$EndSCHEMATC
