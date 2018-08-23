;PINS has external 1K pullups, so we should controll only DDR port of the pin (sink, float)

USI_init:
; float all I2C pins (external pullups)
	cbi PORTB, PIN_SDA ; maybe this should be changed
	cbi PORTB, PIN_SCL
	cbi DDRB, PIN_SDA  ; maybe this should be changed
	cbi DDRB, PIN_SCL

	ldi tmp, (1<<USISIE) | (1<<USIOIE) | (1<<USIWM1) | (1<<USIWM0) | (1<<USICS1)	; Counter overflow is enabled
	mov USICRconst, tmp

	ldi tmp, (1<<USISIF) | (1<<USIOIF) | (1<<USIPF)	; clear start, stop, overflow condition flags and counter
	mov USISRconst, tmp

	ldi tmp,(1<<USISIE) | (0<<USIOIE) | (1<<USIWM1) | (1<<USIWM0) | (1<<USICS1)	; Counter overflow not yet enabled
	out USICR,tmp
	out USISR, USISRconst

	clr USIstate
ret

; when new packet is coming...
USI_start:
	;in r_sreg, SREG
	sbi DDRB, PIN_Vsense
	ldi USIstate, 1	; Status is 1 - start condition detected
	out USICR, USICRconst	; Enable USI Counter overflow interrupt
	; clear USISIF (set to 1) and clear counter
	out USISR, USISRconst	; Clear flags and counter
	;out SREG, r_sreg
	reti

USI_ovf:
	in r_sreg, SREG
	; First, lets check why we came here (Is this is address or a data or something else)
	cpi USIstate,1
	breq I2Cadr
	cpi USIstate,2
	breq I2Cack
	cpi USIstate,3
	;breq I2Cbyte

	;rjmp USIend

	out SREG, r_sreg
	reti

; address byte came
I2Cadr:
	in itmp, USIBR	; read data (address and data direction bit)
	rcall indicateByte
	cpi itmp, 255	;(1<<7) | I2C_ADDRESS
	breq I2CadrOK
	cpi itmp, (0<<7) | I2C_ADDRESS
	breq I2CadrOK
	; not me - leave interrupt
	clr USIstate	; start over
	ldi itmp,(1<<USISIE) | (0<<USIOIE) | (1<<USIWM1) | (1<<USIWM0) | (1<<USICS1)	; Counter overflow not yet enabled
	out USICR,itmp
	out USISR, USISRconst
	;DEBUG
	cbi DDRB, PIN_Vsense
	cbi PORTB, PIN_PWM	; address not ok.
	;rcall indicateByte
	out SREG, r_sreg
	reti
I2CadrOK:
	;DEBUG
	sbi PORTB, PIN_PWM	; address ok.
	cbi DDRB, PIN_Vsense
	; store the direction bit for further action
	mov USIdataDir, itmp
	andi USIdataDir, 0b10000000	; clear all remaining bits as they are not needed
	; first send ack.
	sbi DDRB, PIN_SDA	; set SDA line LOW
	mov itmp, USISRconst
	sbr itmp, 0x0E		;(clear flags and counter set to 0x0E)
	out USISR,itmp
	; now we are ready to wait fot ACK bit
	ldi USIstate, 2
	out SREG, r_sreg
	reti

; Now we are received ACK bit
I2Cack:
	cbi	DDRB, PIN_SDA		; Relase SDA line
	out USISR, USISRconst	; clear flags and counter
	; we are ready to receive/send a byte of data
	;TODO clear data counter
	;
	ldi USIstate, 3			; process send/receive data
	;TODO it seems we can already update USI data register if we want to send data
	out SREG, r_sreg
	reti

; debug
; byte in itmp
indicateByte:
	push itmp
	ldi itmp1, 8 ; 8 bytes
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