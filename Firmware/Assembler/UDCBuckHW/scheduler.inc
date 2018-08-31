; For scheduler we will use Timer0, that overflows every 1ms.
; At 8Mhz divider is 1/64 and overflow value is 125 (OCR0A).
init_Scheduler:
	; Halt the timer
	ldi tmp, (1<<TSM) | (1<<PSR0)
	out GTCCR, tmp		; we don't care about timer1 as GTCCR is not used there
	; Configure timer in CTC mode and prescaller
	ldi tmp, (1<<WGM01) | (0<<WGM00)
	out TCCR0A, tmp
	ldi tmp, (0<<WGM02) | (0<<CS02) | (1<<CS01) | (1<<CS00)
	out TCCR0B, tmp
	out TCNT0, z0
	; Set top value to match 1ms.
	ldi tmp, 125
	out OCR0A, tmp
	; Enable interrupt 
	ldi tmp, (1<<OCIE0A)
	out TIMSK, tmp		; we don't care about timer1 interrupts as we don't use them. 
	
	clr setVolt_tmp		; At the beginning voltage is 0.
	ret
	
; The value transmitted via I2C and stored in EEPROM is human readable value ((V/10)/S)
; But we need to convert it for timer0 use
Scheduler_convert_changeValue:
	; formula is: 1000/Value
	ldi tmp, HIGH(1000)
	mov tmpH1, tmp
	ldi tmp, LOW(1000)
	mov tmpL1, tmp
	clr tmpH2
	lds tmpL2, Voltage_Change
	rcall div16u
	mov V_chg_const, tmpL1
	ret

	
Scheduler_start:
	rcall Scheduler_convert_changeValue
	mov SchedulerCnt, V_chg_const
	; start timer when everything else configured and ready
	ldi tmp, (0<<TSM)
	out GTCCR, tmp		; we don't care about timer1 as GTCCR is not used there
	ret
	
; interrupt for timer0 counting (every 1ms)
TMR0_COMPA:
	; most of the time only these 6 instructions will run
	in r_sreg, SREG
	sbr PWM_flags, (1<<PWM_flag_regulatePWM)	; We adjust PWM every 1ms.
	dec SchedulerCnt
	breq TMR0_changeVolt
	out SREG, r_sreg
	reti
TMR0_changeVolt:
	mov SchedulerCnt, V_chg_const	; rewind counter
	; compare setVolt_tmp register to Voltage_Set
	; if not equal, then adjust setVolt_tmp to be closer to Voltage_Set
	lds itmp, Voltage_Set
	cpse itmp, setVolt_tmp
	rjmp TMR0_changeVolt_adjust
	out SREG, r_sreg
	reti
TMR0_changeVolt_adjust:
	cp setVolt_tmp, itmp
	brlo TMR0_changeVolt_adjust_inc
	dec setVolt_tmp
	out SREG, r_sreg
	reti
TMR0_changeVolt_adjust_inc:
	inc setVolt_tmp
	out SREG, r_sreg
	reti

	