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

	;sbic PINB,PIN_SCL	;Wait for SCL to go low to ensure the "Start Condition" has completed.
	;rjmp PC-1

	ldi USIstate, 1

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
	cpi USIstate, 1
	breq I2Cadr
	cpi USIstate, 2
	breq I2Cack
	cpi USIstate, 3
	breq I2Cbyte

	rjmp USI_exit

I2Cadr:
	; we already have in itmp address and data direction bit
	cpi itmp, (USI_ADDRESS<<1)
	brne I2Cbadadr
	
	sbi DDRB, PIN_Vsense

	ldi YL, low(USI_dataBuffer);Set pointer to the USI array
	ldi YH, high(USI_dataBuffer)

	rcall SetACK

	ldi USIstate, 2
	clr USIbytesCntr
	rjmp USI_exit

I2Cack:
	rcall I2C_clear_ack

	inc USIbytesCntr
	cpi USIbytesCntr, 4	; total bytes to receive (including address byte)
	breq I2C_data_OK

	ldi USIstate, 3
	rjmp USI_exit


I2Cbyte:
	; we already have in itmp address and data direction bit
	st Y+, itmp	; store received byte
	rcall SetACK

	ldi USIstate, 2
	rjmp USI_exit

I2C_data_OK:	; Received all bytes
	sbi portb, PIN_PWM
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
