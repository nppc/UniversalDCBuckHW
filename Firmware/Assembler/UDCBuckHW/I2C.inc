; USIstate flags:
.equ	I2Cadr			= 1	; wait for address
.equ	I2Cack			= 2	; acknowledge received byte
.equ	I2Cdir			= 3	; Read=0, Transmit=1
.equ	I2Cbyte			= 4	; Process byte according to I2Cdir
.equ	I2Ccheckack		= 5	; check acknowledge from master

; called from setup (not onterrupts)
USI_init:
	ldi tmp, (1<<USISIE) | (1<<USIWM1) | (1<<USIWM0) | (1<<USICS1)
	out USICR, tmp
	ldi tmp, (1<<USISIF) | (1<<USIOIF) | (1<<USIPF)
	out USISR, tmp

	; start with all pins floated (pulled up by external resistors)
	;sbi PORTB, PIN_SCL	; should be 1. If Atmel doc says different - don't trust it. :)
	;sbi PORTB, PIN_SDA	; should be always 1.
	;sbi DDRB, PIN_SCL	; should be 1. If Atmel doc says different - don't trust it. :)
	;cbi DDRB, PIN_SDA 
	
	clr USIstate
	clr USIbytesCntr	; clear USI counter
	ret

; Other routines called from interrupts
	
USI_start:
	; SREG register is not affected here

	sbic PINB,PIN_SCL	;Wait for SCL to go low to ensure the "Start Condition" has completed.
	rjmp PC-1

	ldi USIstate, (1<<I2Cadr)	; Initialize flags for new packet

	; Enable USI Counter overflow interrupt
	in itmp,USICR
	sbr itmp, (1<<USIOIE)
	out USICR,itmp
	rcall I2C_clear_ack
	reti

USI_ovf:
	in r_sreg, SREG

	in itmp, USIDR	; read data from buffer
	out USIDR, z0	; clear data register to be prepared for ack.
	
	; Lets check why we came here (Is this is address or a data or something else)
	sbrc USIstate, I2Cadr
	rjmp I2Cadr_check
	sbrc USIstate, I2Cack
	rjmp I2Cack_byte
	sbrc USIstate, I2Ccheckack
	rjmp I2Cack_check
	sbrc USIstate, I2Cbyte
	rjmp I2Cbyte_process

	rjmp USI_exit

I2Cadr_check:
	; we already have in itmp address and data direction bit
	bst itmp,0	; store data direction bit
	andi itmp,  0xFE	; reset data direction bit
	cpi itmp, (USI_ADDRESS<<1)
	brne I2Cbadadr
	cbr USIstate, (1<<I2Cadr)	; Address processed - clear flag
	bld USIstate, I2Cdir	; set flag according to data direction bit
	
	sbi DDRB, PIN_Vsense	; Address OK

	ldi YL, low(USI_dataBuffer);Set pointer to the USI array
	ldi YH, high(USI_dataBuffer)

	rcall SetACK

	sbr USIstate, (1<<I2Cack)
	clr USIbytesCntr
	rjmp USI_exit


I2Cack_byte:	; Acknowledge received byte
	cbr USIstate, (1<<I2Cack)	; reset flag
	rcall I2C_clear_ack

	inc USIbytesCntr
	cpi USIbytesCntr, USI_DATALEN+1	; total bytes to receive (including address byte)
	breq I2C_data_OK
	
	sbr USIstate, (1<<I2Cbyte)	; Receive/Send next byte
	; if we transmit the data, then fill USI data register
	sbrs USIstate, I2Cdir
	rjmp USI_exit
	
	sbi	DDRB, PIN_SDA	; maybe we don't need it
	ld itmp, Y+ ; load data from array
	out USIDR, itmp
	
	rjmp USI_exit


I2Cbyte_process:
	; check, does we read or transmit data?
	sbrc USIstate, I2Cdir
	rjmp I2Cbyte_process_transmit
	
	; we already have data in itmp
	st Y+, itmp	; store received byte
	rcall SetACK

	sbr USIstate, (1<<I2Cack)
	rjmp USI_exit

I2Cbyte_process_transmit:
	; need to wait for ACK from master
	cbi	DDRB, PIN_SDA	; release the line to give master possibility to answer
	; wait for ack bit
	ldi itmp,(1<<USIOIF) | (1<<USIPF) | (1<<USIDC) | (0x0E<<USICNT0)
	out USISR, itmp
	sbr USIstate, (1<<I2Ccheckack)
	rjmp USI_exit

I2Cack_check:
	cbr USIstate, (1<<I2Ccheckack)
	; if ack ok, then prepare to send next byte, if no, then terminate
	; TODO
	; lets asume that ack was OK
	inc USIbytesCntr
	cpi USIbytesCntr, USI_DATALEN+1	; total bytes to receive (including address byte)
	breq I2C_data_OK

	sbi	DDRB, PIN_SDA	; maybe we don't need it
	ld itmp, Y+ ; load data from array
	out USIDR, itmp

	
	rjmp USI_exit	

I2C_data_OK:	; Received all bytes
	sbi PORTB, PIN_PWM
	; finish receiving

I2Cbadadr:
	rcall StopDetected

USI_exit:
	out SREG, r_sreg
	reti

	
SetACK:
	sbi	DDRB, PIN_SDA
	sbi PortB, PIN_SCL
	sbi DDRB, PIN_SCL

	ldi itmp,(1<<USIOIF) | (1<<USIPF) | (1<<USIDC) | (0x0E<<USICNT0)
	out USISR, itmp
	ret


StopDetected:

	in itmp, USICR;(Disable USI Counter overflow interrupt)
	cbr itmp, 0b01000000
	out USICR, itmp

I2C_clear_ack:

	cbi	DDRB, PIN_SDA
	cbi PortB, PIN_SCL
	cbi DDRB, PIN_SCL

	ldi itmp, (1<<USISIF) | (1<<USIOIF) | (1<<USIPF)
	out USISR, itmp
	ret
;------------------
