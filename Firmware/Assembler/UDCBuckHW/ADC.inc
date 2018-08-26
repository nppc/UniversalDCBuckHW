; We will have priority for V measuring.
; We measure first 8 samples of V.
; Then measure 1 sample of I.
; So, I will be measured 8 times slower than V.
;
; Variable ADC_counter:
; 0 - skip ADC reading
; 1-8 - Read Voltage
; 8 - after reading, change ADC channel to I
; 9 - skip ADC reading
; 10 - Read Current
; 10 - after reading, change ADC channel to V and reset counter

.EQU 	VSENSE_ADC = (0<<MUX3) | (0<<MUX2) | (1<<MUX1) | (0<<MUX0) ; ADC2 (PB4) 
.EQU 	ISENSE_ADC = (0<<MUX3) | (0<<MUX2) | (1<<MUX1) | (1<<MUX0) ; ADC3 (PB3)
;.EQU 	INTTEMP_ADC = (1<<MUX3) | (1<<MUX2) | (1<<MUX1) | (1<<MUX0) ; ADC4 (Internal temperature sensor)


; ADC conversion complete interrupt
ADC_INT:
	in r_sreg, SREG
	; first read the result. We will start new conversion later as we have one ADC clock time for it.
	in itmp1, ADCL
	in itmp2, ADCH
	; should we change channel?
	cpi ADC_counter, 8
	brne ADC_skp_change_1
	; Change channel to I
	ldi itmp, ISENSE_ADC
	out ADMUX, itmp
ADC_skp_change_1:
	cpi ADC_counter, 10
	brne ADC_skp_change_2
	; Change channel to V
	ldi itmp, VSENSE_ADC
	out ADMUX, itmp
ADC_skp_change_2:
	; start new ADC reading
	sbi ADCSRA, ADSC
	; continue to analyze counter
	cpi ADC_counter, 0	; skip reading
	breq adc_int_exit
	cpi ADC_counter, 9	; 1-8
	brlo ADC_read_Volt
	cpi ADC_counter, 9 ; skip reading
	breq adc_int_exit
	cpi ADC_counter, 10 
	breq ADC_read_Current

adc_int_exit:
	inc ADC_counter
	out SREG, r_sreg
	reti
ADC_read_Volt:
	; store value for future reading in main loop
	sts ADC_Voltage_RAW, itmp1
	sts ADC_Voltage_RAW+1, itmp2
	rjmp adc_int_exit
ADC_read_Current:
	; store value for future reading in main loop
	sts ADC_Current_RAW, itmp1
	sts ADC_Current_RAW+1, itmp2
	; also reset counter
	ldi ADC_counter, 255	; Before exit it will became 0.
	rjmp adc_int_exit


init_ADC:
	cli	; just in case
	; Initialize remaining pins
	#ifndef DEBUG	; disable digital logic only in "prodution"
		ldi tmp, (1<<PIN_Vsense) | (1<<PIN_Isense)
		out DIDR0, tmp		; disable digital logic
	#endif
	cbi DDRB, PIN_Vsense	; input
	cbi DDRB, PIN_Isense	; input
	; Initialize ADC variables
	clr ADC_counter
	; set 125khz clock for ADC (8mhz/64)m interrupt
	ldi tmp, (ADPS2<<1) | (ADPS1<<1) | (ADPS0<<0) | (ADIE<<1) | (ADEN<<1)
	out ADCSRA, tmp
	; configure channel 
	; voltage reference by default is VCC
	#ifdef MOVINGAVERAGE
		; fill M_average array for I
		ldi tmp, ISENSE_ADC
		out ADMUX, tmp
		ldi ZL, LOW(M_AVERAGE_current_COUNTER)
		ldi ZH, HIGH(M_AVERAGE_current_COUNTER)
		rcall init_ADC_array
	#endif
	ldi tmp, VSENSE_ADC
	out ADMUX, tmp
	#ifdef MOVINGAVERAGE
		; fill M_average array for V
		ldi ZL, LOW(M_AVERAGE_voltage_COUNTER)
		ldi ZH, HIGH(M_AVERAGE_voltage_COUNTER)
		rcall init_ADC_array
		; exit from ADC init.
		; now V channel is selected
	#endif
	; clear ADC interrupt flag
	sbi ADCSRA, ADIF
	; also reset SRAM variables for RAW values (they will be compared in main loop to know, that new value available)
	; we update only High byte to 255
	ldi tmp, 255
	sts ADC_Voltage_RAW+1, tmp
	sts ADC_Current_RAW+1, tmp
	ret

#ifdef MOVINGAVERAGE
init_ADC_array:
	ldi tmp, MOVINGAVERAGE_N * 2
init_ADC_loop:
	push tmp
	push ZL
	push ZH
	; start first conversion
	sbi ADCSRA, ADSC
	; wait for result
waitforADC:
	sbic ADCSRA, ADSC
	rjmp waitforADC
	; read ADC
	in tmp2, ADCL
	in tmp3, ADCH
	rcall moving_average	; fill array
	pop ZH
	pop ZL
	pop tmp
	dec tmp
	brne init_ADC_loop
	ret
#endif
