; PWM on OC1A pin (PB1)
; 250 Khz
init_PWM:
	cbi PORTB, PIN_PWM		; after reset it will be LOW,( but still, let's force it to LOW.
	sbi DDRB, PIN_PWM		; output
	out TCNT1, z0
	out OCR1A, z0			; no PWM
	clr PWM_flags
	sts PWM_Value, z0
	ldi tmp, (0<<CS13) | (0<<CS12) | (0<<CS11) | (1<<CS10) | (1<<PWM1A) | (1<<COM1A1) | (0<<COM1A0)	; prescaller 1; fast PWM on OC1A
	out TCCR1, tmp	
	; Enable 64mhz
	ldi tmp, (1<<PLLE)
	out PLLCSR, tmp
	; wait until clock stabilize (about 100us)
waitPLL:
	in tmp, PLLCSR
	sbrs tmp, PLOCK
	rjmp waitPLL
	ldi tmp, (1<<PCKE) | (1<<PLLE)
	out PLLCSR, tmp
	ret