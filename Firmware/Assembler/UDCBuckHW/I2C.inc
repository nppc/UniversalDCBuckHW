;PINS has external 1K pullups, so we should controll only DDR port of the pin (sink, float)

USI_init:

	;ldi tmp, (1<<USISIE) | (1<<USIOIE) | (1<<USIWM1) | (1<<USIWM0) | (1<<USICS1)	; Counter overflow is enabled
	;mov USICRconst, tmp

	;ldi tmp, (1<<USISIF) | (1<<USIOIF) | (1<<USIPF)	; clear start, stop, overflow condition flags and counter
	;mov USISRconst, tmp

	;ldi tmp,(1<<USISIE) | (0<<USIOIE) | (1<<USIWM1) | (1<<USIWM0) | (1<<USICS1)	; Counter overflow not yet enabled
	;out USICR,tmp
	;out USISR, USISRconst

	cbi PORTB, PIN_SCL
	cbi DDRB, PIN_SCL
	cbi PORTB, PIN_SDA ; maybe need to change
	cbi DDRB, PIN_SDA 

	ldi tmp,(1<<USISIE)+(1<<USIWM1)+(1<<USIWM0)+(1<<USICS1) ;+(1<<USICS0)
	out USICR,tmp
	ldi tmp,0xf0
	out USISR,tmp

	;sbi PORTB, PIN_SCL
	;sbi DDRB, PIN_SCL
	;sbi PORTB, PIN_SDA
	;cbi DDRB, PIN_SDA

	clr USIstate
	ret

; when new packet is coming...
USI_start:
	sbi DDRB, PIN_Vsense
	ldi USIstate, 1
	in itmp,USICR;(Enable USI Counter overflow interrupt)
	sbr itmp,0b01000000
	out USICR,itmp
	rcall I2C_clear_ack
	;ldi tmp,0xf0
	reti
	;sbi DDRB, PIN_Vsense
	;ldi USIstate, 1	; Status is 1 - start condition detected
	;out USICR, USICRconst	; Enable USI Counter overflow interrupt
	;rcall I2C_clear_ack
	;reti

USI_ovf:
	in r_sreg, SREG
	; First, lets check why we came here (Is this is address or a data or something else)
	cpi USIstate,1
	breq I2Cadr
	cpi USIstate,2
	breq I2Cack
	cpi USIstate,3
	breq I2Cbyte

	;rjmp USIend

	out SREG, r_sreg
	reti

; address byte came
I2Cadr:
	in itmp, USIDR	; read data (address and data direction bit)
	;rcall indicateByte
;	cpi itmp, ((I2C_ADDRESS << 1) & 0xFE)	; writing
;	breq I2CadrOK
;	cpi itmp, ((I2C_ADDRESS << 1) | 1)	; reading
;	breq I2CadrOK
	cpi itmp, (I2C_ADDRESS & 0xFE)	; writing
	breq I2CadrOK
	cpi itmp, (I2C_ADDRESS | 1)	; reading
	breq I2CadrOK
	; not me - leave interrupt
	clr USIstate	; start over
	;ldi itmp,(1<<USISIE) | (0<<USIOIE) | (1<<USIWM1) | (1<<USIWM0) | (1<<USICS1)	; Counter overflow not yet enabled
	;out USICR,itmp
	in itmp,USICR;(Disable USI Counter overflow interrupt)
	cbr itmp,0b01000000
	out USICR,itmp
	rcall I2C_clear_ack
	;DEBUG
	cbi DDRB, PIN_Vsense
	cbi PORTB, PIN_PWM	; address not ok.
	;rcall indicateByte
	out SREG, r_sreg
	reti
I2CadrOK:
	;DEBUG
	sbi PORTB, PIN_PWM	; address ok.
	
	; store the direction bit for further action
	mov USIdataDir, itmp
	andi USIdataDir, 0b00000001	; clear all remaining bits as they are not needed
	; first send ack.
	rcall I2C_set_ack
	; now we are ready to wait fot ACK bit
	ldi USIstate, 2
	out SREG, r_sreg
	reti

; Now we are received ACK bit
I2Cack:
	;DEBUG
	cbi DDRB, PIN_Vsense

	rcall I2C_clear_ack
	;cbi	DDRB, PIN_SDA		; Relase SDA line
	;out USISR, USISRconst	; clear flags and counter
	; we are ready to receive/send a byte of data
	;TODO clear data counter
	;
	ldi USIstate, 3			; process send/receive data
	;TODO it seems we can already update USI data register if we want to send data
	out SREG, r_sreg
	reti

I2Cbyte:
	in itmp,USIBR
	push itmp
	rcall I2C_set_ack
	clr USIstate
	; end receiving
	in itmp,USICR;(Disable USI Counter overflow interrupt)
	cbr itmp,0b01000000
	out USICR,itmp
	rcall I2C_clear_ack
	cbi PORTB, PIN_PWM
	pop itmp
	rcall indicateByte

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


; debug
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