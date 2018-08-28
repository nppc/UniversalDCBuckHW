;
; UDCBuckHW.asm
;
; Created: 8/13/2018 7:17:48 PM
; Author : Pavel
;

#define DEBUG

#define	MOVINGAVERAGE ; comment it if not needed
.EQU	MOVINGAVERAGE_N = 5 ; can be 3, 5 or 7.

.EQU	USI_ADDRESS	= 0x5E	; choose address here (7bit)
.EQU	USI_DATALEN = 6		; bytes to receive/send via I2C (not including address byte)

.include "tn85def.inc"

; Pins assignment
.EQU	PIN_SDA		= PB0	; I2C_SDA
.EQU	PIN_PWM		= PB1	; PWM to Buck mosfet driver
.EQU	PIN_SCL		= PB2	; I2C_SCL
.EQU	PIN_Isense	= PB3	; Current measurement (analog signal)
.EQU	PIN_Vsense	= PB4	; Voltage measurement (analog signal)


; variables assignment
.def	z0			=	r0
.def	z1			=	r1
.def	r_sreg		=	r2	; Store SREG register in interrupts
.def	tmp			=	r16
.def	tmp1		=	r17
.def	tmp2		=	r3
.def	tmp3		=	r4
.def	tmp4		=	r5
.def	itmp		=	r18	; variables to use in interrupts
.def	itmp1		=	r19	; variables to use in interrupts
.def	itmp2		=	r6	; variables to use in interrupts
.def	tmpL1		=	r7	; temp register for 16 bit calculations
.def	tmpH1		=	r8	; temp register for 16 bit calculations
.def	tmpL2		=	r9	; temp register for 16 bit calculations
.def	tmpH2		=	r10	; temp register for 16 bit calculations
.def	USIstate	=	r20	; state of I2C protocol
.def	USIdataDir	=	r21	; Data direction flag (actually only 7th bit) 
.def 	USIbytesCntr=	r22	; Counter for receiving/sending bytes via USI
.def	ADC_counter	=	r23	; Flags for ADC. Refer to ADC.inc for details
; YH:YL are used in USI interrupt as a pointer to the SRAM buffer
.DSEG
.ORG SRAM_START
USI_dataBuffer:				.BYTE USI_DATALEN	; USI bytes buffer
#ifdef MOVINGAVERAGE
M_AVERAGE_voltage_COUNTER:	.BYTE 1	 ; Counter in the table
M_AVERAGE_voltage_TABLE:	.BYTE MOVINGAVERAGE_N * 2 ; Table for running moving average algorithm (max 14 bytes).
M_AVERAGE_current_COUNTER:	.BYTE 1	 ; Counter in the table
M_AVERAGE_current_TABLE:	.BYTE MOVINGAVERAGE_N * 2 ; Table for running moving average algorithm (max 14 bytes).
#endif
ADC_Voltage_RAW:			.BYTE 2	; Raw ADC value for Voltage
ADC_Current_RAW:			.BYTE 2	; Raw ADC value for Current


.CSEG
.ORG 0

	; Interrupt vectors
	rjmp RESET ; Reset Handler
	reti	;rjmp EXT_INT0 ; IRQ0 Handler
	reti	;PCINT0 External Interrupt Request 1
	reti	;TIMER1 COMPA Timer/Counter1 Compare Match A
	reti	;TIMER1 OVF Timer/Counter1 Overflow
	reti	;TIMER0 OVF Timer/Counter0 Overflow
	reti	;EE_RDY EEPROM Ready
	reti	;ANA_COMP Analog Comparator
	rjmp ADC_INT ;ADC Conversion Complete
	reti	;TIMER1 COMPB Timer/Counter1 Compare Match B
	reti	;TIMER0 COMPA Timer/Counter0 Compare Match A
	reti	;TIMER0 COMPB Timer/Counter0 Compare Match B
	reti	;WDT
	rjmp USI_start	;USI start
	rjmp USI_ovf	;USI Overflow

	
.include "I2C.inc"
.include "PWM.inc"
.include "ADC.inc"
.include "MovAverage.inc"
.include "math.inc"

RESET:
	cli

	;initialize constants
	clr z0
	clr z1
	inc z1
	
	ldi tmp, 1<<CLKPCE	
	out CLKPR, tmp		; enable clock change
	out CLKPR, z0		; prescaler 1

	ldi tmp, high(RAMEND) 
	out SPH,tmp				; Set Stack Pointer to top of RAM
	ldi tmp, low(RAMEND)
	out SPL,tmp				; Set Stack Pointer to top of RAM

	rcall init_PWM	; Initialize FET controlling with PWM
	#ifdef MOVINGAVERAGE
		rcall init_Moving_Average
	#endif
	rcall init_ADC	; Initialize V and I measuring
	rcall USI_init	; initialize registers and pins for I2C

	#ifdef DEBUG
		sbi DDRB, PIN_PWM
	#endif
	
	sei
	
loop:
	rjmp loop

	; reading of ADC value looks like this:
	; lds tmp3, ADC_Voltage_RAW+1
	; cpi tmp3, 255
	; breq Value_not_ready
	; lds tmp2, ADC_Voltage_RAW
	; rcall moving_average
	; work with value


#ifdef DEBUG
	delay500ms:
		ldi  tmp, 22
		ldi  tmp1, 100
		clr  tmp2
	delay500L1:
		dec  tmp2
		brne delay500L1
		dec  tmp1
		brne delay500L1
		dec  tmp
		brne delay500L1
		ret
		
	delay100ms:
		ldi  tmp, 0
		ldi  tmp1, 0
	delay100L1: 
		dec  tmp1
		brne delay100L1
		dec  tmp
		brne delay100L1
		ret
#endif

#ifdef DEBUG
	; byte in tmp3
	indicateByte:
		rcall delay100ms
		cbi PORTB, PIN_PWM
		rcall delay100ms
		sbi PORTB, PIN_PWM
		rcall delay100ms
		cbi PORTB, PIN_PWM
		rcall delay100ms
		sbi PORTB, PIN_PWM
		rcall delay100ms
		cbi PORTB, PIN_PWM
		rcall delay500ms
		rcall delay500ms
		ldi tmp, 8 ; 8 bits
	indicateBloop:
		push tmp
		lsr tmp3
		brcs show1
		sbi PORTB, PIN_PWM
		rcall delay100ms	; show0
		cbi PORTB, PIN_PWM
	indicateCont:
		rcall delay500ms; pause
		pop tmp
		dec tmp
		brne indicateBloop
		rcall delay100ms
		ret
	show1:
		sbi PORTB, PIN_PWM
		rcall delay500ms	; show1
		cbi PORTB, PIN_PWM
		rjmp indicateCont
#endif
