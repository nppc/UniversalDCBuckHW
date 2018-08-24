;
; UDCBuckHW.asm
;
; Created: 8/13/2018 7:17:48 PM
; Author : Pavel
;

#define DEBUG

#define	MOVINGAVERAGE ; comment it if not needed
.EQU	MOVINGAVERAGE_N_voltage = 5 ; can be 3, 5 or 7.
.EQU	MOVINGAVERAGE_N_current = 5 ; can be 3, 5 or 7.

.EQU	USI_ADDRESS	= 0x5E	; choose address here (7bit)
.EQU	USI_DATALEN = 2		; currently we receive only 2 bytes

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
.def	tmpL1		=	r6	; temp register for 16 bit calculations
.def	tmpH1		=	r7	; temp register for 16 bit calculations
.def	tmpL2		=	r8	; temp register for 16 bit calculations
.def	tmpH2		=	r9	; temp register for 16 bit calculations
.def	USIstate	=	r20	; state of I2C protocol
.def	USICRconst	=	r10	; predevined interrupts settings for starting counter overflow interrupt
.def	USISRconst	=	r11	; predevined clearing start flag and counter 
.def	USIdataDir	=	r21	; Data direction flag (actually only 7th bit) 
.def 	USIbytesCntr=	r22	; Counter for receiving/sending bytes via USI
; YH:YL are used in USI interupt as a pointer to the SRAM buffer
.DSEG
.ORG SRAM_START
USI_dataBuffer:				.BYTE USI_DATALEN	; USI bytes buffer
#ifdef MOVINGAVERAGE
M_AVERAGE_voltage_TABLE:	.BYTE MOVINGAVERAGE_N_voltage * 2 ; Table for running moving average algorithm (max 14 bytes).
M_AVERAGE_voltage_COUNTER:	.BYTE 1	 ; Counter in the table
M_AVERAGE_current_TABLE:	.BYTE MOVINGAVERAGE_N_current * 2 ; Table for running moving average algorithm (max 14 bytes).
M_AVERAGE_current_COUNTER:	.BYTE 1	 ; Counter in the table
#endif


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
	reti	;ADC ADC Conversion Complete
	reti	;TIMER1 COMPB Timer/Counter1 Compare Match B
	reti	;TIMER0 COMPA Timer/Counter0 Compare Match A
	reti	;TIMER0 COMPB Timer/Counter0 Compare Match B
	reti	;WDT
	rjmp USI_start	;USI start
	rjmp USI_ovf	;USI Overflow

.include "I2C.inc"


RESET:
	cli

	ldi tmp, high(RAMEND) 
	out SPH,tmp				; Set Stack Pointer to top of RAM
	ldi tmp, low(RAMEND)
	out SPL,tmp				; Set Stack Pointer to top of RAM

	;initialize constants
	clr z0
	clr z1
	inc z1
	
	clr USIbytesCntr	; clear USI counter

	ldi tmp, 1<<CLKPCE	
	out CLKPR, tmp		; enable clock change
	out CLKPR, z0		; prescaler 1

	rcall USI_init	; initialize registers and pins

	; Initialize remaining pins
	cbi PORTB, PIN_PWM		; after reset it will be LOW, but still, let's force it to LOW.
	sbi DDRB, PIN_PWM		; output
	cbi DDRB, PIN_Vsense	; input
	cbi DDRB, PIN_Isense	; input

	sei

loop:
	cpi USIbytesCntr,USI_DATALEN
	brne loop	; not yet
	; now indicate the received data
	cli
	ldi YH, HIGH(USI_dataBuffer);
	ldi YL, LOW(USI_dataBuffer);
	ld tmp3, Y+
	rcall indicateByte
	ld tmp3, Y+
	rcall indicateByte
	rcall delay500ms
	rcall delay500ms
	rcall delay500ms
	rcall delay500ms
	rcall delay500ms
	rcall delay500ms
	sei
	rjmp loop

#ifdef DEBUG
	delay500ms:
		ldi  tmp, 22
		ldi  tmp1, 120
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
		rcall delay500ms
		rcall delay500ms
		rcall delay500ms
		ret
	show1:
		sbi PORTB, PIN_PWM
		rcall delay500ms	; show1
		cbi PORTB, PIN_PWM
		rjmp indicateCont
#endif
