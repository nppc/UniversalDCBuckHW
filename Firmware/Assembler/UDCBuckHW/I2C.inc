;PINS has external 1K pullups, so we should controll only DDR port of the pin (sink, float)

USI_init:

	ldi tmp, (1<<USISIE) | (1<<USIOIE) | (1<<USIWM1) | (1<<USIWM0) | (1<<USICS1)	; Counter overflow is enabled
	mov USICRconst, tmp

	ldi tmp, (1<<USISIF) | (1<<USIOIF) | (1<<USIPF)	; clear start, stop, overflow condition flags and counter
	mov USISRconst, tmp

	;ldi tmp,(1<<USISIE) | (0<<USIOIE) | (1<<USIWM1) | (1<<USIWM0) | (1<<USICS1)	; Counter overflow not yet enabled
	;out USICR,tmp
	;out USISR, USISRconst

	ldi tmp,(1<<USISIE)+(1<<USIWM1)+(1<<USIWM0)+(1<<USICS1)
	out USICR,tmp
	out USISR, USISRconst	;0xF0

	sbi PORTB, PIN_SCL	; should be 1. If Atmel doc says different - don't trust it. :)
	sbi PORTB, PIN_SDA	; should be always 1.
	sbi DDRB, PIN_SCL	; should be 1. If Atmel doc says different - don't trust it. :)
	cbi DDRB, PIN_SDA 

	clr USIstate
	ret

; when new packet is coming...
USI_start:
	; SREG register is not affected here
	#ifdef DEBUG
		sbi DDRB, PIN_Vsense
	#endif
	cbi DDRB, PIN_SDA 	; just in case it was changed in ACK routine
	ldi USIstate, 1

	sbic PINB,PIN_SCL	;Wait for SCL to go low to ensure the "Start Condition" has completed.
	rjmp PC-1
	
	; we are not checking here for stop condition. Maybe implement it in the future.
	
	out USICR, USICRconst	;(1<<USISIE)|(1<<USIOIE)|(1<<USIWM1)|(1<<USIWM0)|(1<<USICS1)
   	out USISR, USISRconst	;(1<<USISIF)|(1<<USIOIF)|(1<<USIPF)

	;in itmp,USICR;(Enable USI Counter overflow interrupt)
	;sbr itmp,0b01000000
	;out USICR,itmp
	;rcall I2C_clear_ack
	;ldi tmp,0xf0
	;reti
	;sbi DDRB, PIN_Vsense
	;ldi USIstate, 1	; Status is 1 - start condition detected
	;out USICR, USICRconst	; Enable USI Counter overflow interrupt
	;rcall I2C_clear_ack
	
	reti

USI_ovf:
	in itmp, USIDR	; read data from buffer
	out USIDR, z0	; clear data register to be prepared for ack.
	in r_sreg, SREG
	#ifdef DEBUG
		cpse USIbytesCntr,z0
		cbi DDRB, PIN_Vsense
	#endif

	; First, lets check why we came here (Is this is address or a data or something else)
	cpi USIstate,1
	breq I2Cadr
	cpi USIstate,2
	breq I2Cack
	cpi USIstate,3
	breq I2Cbyte

	;rjmp USIend
	in itmp,USICR;(Disable USI Counter overflow interrupt)
	cbr itmp,0b01000000
	out USICR,itmp
	
	out SREG, r_sreg
	reti

; address byte came
I2Cadr:
	; we already have in itmp address and data direction bit
;	#ifdef DEBUG
;		push itmp
;		rcall indicateByte
;		pop itmp
;	#endif
	cpi itmp, ((USI_ADDRESS << 1) & 0xFE)	; writing
	breq I2CadrOK
	cpi itmp, ((USI_ADDRESS << 1) | 1)	; reading
	breq I2CadrOK
	; not me - leave interrupt
	clr USIstate	; start over
	;ldi itmp,(1<<USISIE) | (0<<USIOIE) | (1<<USIWM1) | (1<<USIWM0) | (1<<USICS1)	; Counter overflow not yet enabled
	;out USICR,itmp
	in itmp,USICR;(Disable USI Counter overflow interrupt)
	cbr itmp,0b01000000
	out USICR,itmp
	rcall I2C_clear_ack
	#ifdef DEBUG
		cbi DDRB, PIN_Vsense
		cbi PORTB, PIN_PWM	; address not ok.
	#endif
	;rcall indicateByte
	out SREG, r_sreg
	reti
I2CadrOK:
	;DEBUG
	#ifdef DEBUG
		sbi PORTB, PIN_PWM	; address ok.
	#endif
	; store the direction bit for further action
	mov USIdataDir, itmp
	andi USIdataDir, 0b00000001	; clear all remaining bits as they are not needed
	clr USIbytesCntr	; counter for receiving bytes
	; initialize buffer
	ldi YH, HIGH(USI_dataBuffer);
	ldi YL, LOW(USI_dataBuffer);
	; now we are ready to wait fot ACK bit
	rcall I2C_set_ack
	ldi USIstate, 2
	out SREG, r_sreg
	reti

; Now we are received ACK bit
I2Cack:
;	#ifdef DEBUG
;		cbi DDRB, PIN_Vsense
;	#endif

	rcall I2C_clear_ack
	;cbi	DDRB, PIN_SDA		; Relase SDA line
	;out USISR, USISRconst	; clear flags and counter
	; we are ready to receive/send a byte of data
	ldi USIstate, 3			; process send/receive data
	; Maybe we are finished to receive all data?
	cpi USIbytesCntr, USI_DATALEN
	brne need_more_data
	; all data received
	; TODO: set the flag to indicate it.
	; reset the USI
	in itmp,USICR;(Disable USI Counter overflow interrupt)
	cbr itmp,0b01000000
	out USICR,itmp	
	#ifdef DEBUG
		cbi PORTB, PIN_PWM	; finished receiving
	#endif

need_more_data:
	out SREG, r_sreg
	reti

I2Cbyte:
	;#ifdef DEBUG
	;	cbi DDRB, PIN_Vsense
	;#endif
	; We already has a byte in itmp
	; store byte to the buffer
	st Y+,itmp
	inc USIbytesCntr
	rcall I2C_set_ack
	ldi USIstate, 2
	out SREG, r_sreg
	reti


I2C_clear_ack:
	cbi	DDRB, PIN_SDA
	cbi PORTB, PIN_SCL
	cbi DDRB, PIN_SCL
	in itmp,USISR
	cbr itmp,0b00001111;(Clear counter)
	sbr itmp,0b11100000;(Enable new interrupts. Releasing SCL)
	out USISR,itmp
	;out USISR, USISRconst	; Clear flags and counter

	ret

I2C_set_ack:
	sbi	DDRB, PIN_SDA
	sbi PORTB, PIN_SCL
	sbi DDRB, PIN_SCL

	in itmp,USISR
	cbr itmp,0b00000001;(Set counter to 0xE)
	sbr itmp,0b11101110;(Enable new interrupts. Releasing SCL)
	out USISR,itmp
;	mov itmp, USISRconst
;	sbr itmp, 0x0E		;(clear flags and counter set to 0x0E)
;	out USISR, itmp
	ret


#ifdef DEBUG1
	; byte in itmp
	indicateByte:
		rcall delay500ms
		push itmp
		ldi itmp1, 8 ; 8 bits
	indicateBloop:
		lsr itmp
		brcs show1
		sbi PORTB, PIN_PWM
		rcall delay100ms	; show0
		cbi PORTB, PIN_PWM
	indicateCont:
		rcall delay500ms; pause
		dec itmp1
		brne indicateBloop
		rcall delay500ms
		rcall delay500ms
		rcall delay500ms
		pop itmp
		ret
	show1:
		sbi PORTB, PIN_PWM
		rcall delay500ms	; show1
		cbi PORTB, PIN_PWM
		rjmp indicateCont
#endif