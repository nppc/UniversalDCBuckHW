; We will have priority for V measuring.
; We measure first 8 samples of V.
; Then measure 1 sample of I.
; So, I will be measured 8 times slower than V.
;
; ADC_flags:
; bits 0-3 - counter:
;  0 - skip ADC reading
;  1-8 - Read Voltage
;  8 - after reading, change ADC channel to I
;  9 - skip ADC reading
;  10 - Read Current
;  10 - after reading, change ADC channel to V and reset counter
; bit 4 - N/A
; bit 5 - N/A
; bit 6 - Voltage ADC value ready
; bit 7 - Current ADC value ready
.EQU flag_ADCcounter = 0b00001111
.EQU flag_ADCvoltageReady = 6
.EQU flag_ADCcurrentReady = 7

.EQU 	VSENSE_ADC = (0<<MUX3) | (0<<MUX2) | (1<<MUX1) | (0<<MUX0) ; ADC2 (PB4) 
.EQU 	ISENSE_ADC = (0<<MUX3) | (0<<MUX2) | (1<<MUX1) | (1<<MUX0) ; ADC3 (PB3)
;.EQU 	INTTEMP_ADC = (1<<MUX3) | (1<<MUX2) | (1<<MUX1) | (1<<MUX0) ; ADC4 (Internal temperature sensor)

.EQU	VOLT_DIV_CONST		= 155		; To get this number use formula (for 21v max): 
										; VREF=5v 
										; 4095/(Vmax*10)*8, where Vmax=(R1+R2)*VREF/R2
										; and resistor values is from divider (22K/6.8K)
										; Vmax=(22+6.8)*VREF/6.8=21.18
										; 4095/(21.18*10)*8=155
										; And then voltage will be calculated as ADC * 32 / VOLT_DIV_CONST

.EQU	CURRENT_DIV_CONST	= 242		; To get this number use formula (for 185mV/A sensor): 
										; VREF=5v 
										; MULCOEFF=VREF*1000/185/1023*10
										; 1/MULCOEFF*64=242
										; And then current will be calculated as (ADC-ADC_Current_zero_RAW)*64/CURRENT_DIV_CONST
										

; ADC conversion complete interrupt
ADC_INT:
	in r_sreg, SREG
	; first read the result. We will start new conversion later as we have one ADC clock time for it.
	in itmp2, ADCL
	in itmp3, ADCH
	; get counter from ADC flag
	mov itmp1, ADC_flags
	andi itmp1, flag_ADCcounter
	; should we change channel?
	cpi itmp1, 8	; check counter
	brne ADC_skp_change_1
	; Change channel to I
	ldi itmp, ISENSE_ADC
	out ADMUX, itmp
	rjmp ADC_skp_change_2
ADC_skp_change_1:
	cpi itmp1, 10	; check counter
	brne ADC_skp_change_2
	; Change channel to V
	ldi itmp, VSENSE_ADC
	out ADMUX, itmp
ADC_skp_change_2:
	; start new ADC reading
	sbi ADCSRA, ADSC
	; continue to analyze counter
	cpi itmp1, 0	; skip reading
	breq adc_int_exit
	cpi itmp1, 9	; 1-8
	brlo ADC_read_Volt
	cpi itmp1, 9 ; skip reading
	breq adc_int_exit
	cpi itmp1, 10 
	breq ADC_read_Current

adc_int_exit:
	inc itmp1	; increment counter
	; store counter back to flags register
	cbr ADC_flags, flag_ADCcounter	; clear old counter value
	or ADC_flags, itmp1			; store new counter value
	out SREG, r_sreg
	reti
ADC_read_Volt:
	; store value for future reading in main loop
	sts ADC_Voltage_RAW, itmp2
	sts ADC_Voltage_RAW+1, itmp3
	sbr ADC_flags, (1<<flag_ADCvoltageReady)
	rjmp adc_int_exit
ADC_read_Current:
	; store value for future reading in main loop
	sts ADC_Current_RAW, itmp2
	sts ADC_Current_RAW+1, itmp3
	sbr ADC_flags, (1<<flag_ADCcurrentReady)
	; also reset counter
	ldi itmp1, 255	; Before exit counter will became 0.
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
	clr ADC_flags
	; set 125khz clock for ADC (8mhz/64)m interrupt
	ldi tmp, (1<<ADPS2) | (1<<ADPS1) | (0<<ADPS0) | (1<<ADIE) | (1<<ADEN)
	out ADCSRA, tmp
	; configure channel 
	; voltage reference by default is VCC
	ldi tmp, ISENSE_ADC
	out ADMUX, tmp
	#ifdef MOVINGAVERAGE
		; fill M_average array for I
		ldi ZL, LOW(M_AVERAGE_current_COUNTER)
		ldi ZH, HIGH(M_AVERAGE_current_COUNTER)
		rcall init_ADC_array
	#else
		; we need at least one adc conversion to have reference
		ldi tmp, 10	; read 10 samples
	waitforADC_Ref_L1:
		sbi ADCSRA, ADSC
		; wait for result
	waitforADC_Ref:
		sbic ADCSRA, ADSC
		rjmp waitforADC_Ref
		in tmp2, ADCL
		in tmp3, ADCH
		dec tmp
		brne waitforADC_Ref_L1
	#endif
	; store last value to get reference for Current measurement (0.0A)
	sts ADC_Current_zero_RAW, itmp2
	sts ADC_Current_zero_RAW+1, itmp3

	ldi tmp, VSENSE_ADC
	out ADMUX, tmp
	#ifdef MOVINGAVERAGE
		; fill M_average array for V
		ldi ZL, LOW(M_AVERAGE_voltage_COUNTER)
		ldi ZH, HIGH(M_AVERAGE_voltage_COUNTER)
		rcall init_ADC_array
		; now V channel is selected
	#endif
	; clear ADC interrupt flag
	sbi ADCSRA, ADIF
	; no sense to store voltage now
	;sts ADC_Voltage_RAW, itmp2
	;sts ADC_Voltage_RAW+1, itmp3
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
