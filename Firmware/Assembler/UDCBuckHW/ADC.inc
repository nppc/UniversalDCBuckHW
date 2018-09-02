; We will have priority for V measuring.
; We measure first 8 samples of V.
; Then measure 1 sample of I.
; So, I will be measured 8 times slower than V.
;
; ADC_counter:
;  0 - skip ADC reading
;  1-8 - Read Voltage
;  8 - after reading, change ADC channel to I
;  9 - skip ADC reading
;  10 - Read Current
;  10 - after reading, change ADC channel to V and reset counter

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
										

ADC_Read:
	in tmp2, ADCL
	in tmp3, ADCH
	cpi ADC_counter, 8	; check counter
	brne ADC_skp_change_1
	; Change channel to I
	ldi tmp, ISENSE_ADC
	out ADMUX, tmp
	rjmp ADC_skp_change_2
ADC_skp_change_1:
	cpi ADC_counter, 10	; check counter
	brne ADC_skp_change_2
	; Change channel to V
	ldi tmp, VSENSE_ADC
	out ADMUX, tmp
ADC_skp_change_2:
	; start new ADC reading
	sbi ADCSRA, ADSC
	; continue to analyze counter
	cpi ADC_counter, 0	; skip reading
	breq adc_exit
	cpi ADC_counter, 9	; 1-8
	brlo ADC_read_Volt
	cpi ADC_counter, 9 ; skip reading
	breq adc_exit
	cpi ADC_counter, 10 
	breq ADC_read_Current

adc_exit:
	inc ADC_counter	; increment counter
	ret
ADC_read_Volt:
	; store value
	cli
	sts USI_dataBuffer+7, tmp2
	sts USI_dataBuffer+8, tmp3
	sei
	rcall Convert_VoltageADC_to_Volt
	rjmp adc_exit
ADC_read_Current:
	; store value
	cli
	sts USI_dataBuffer+9, tmp2
	sts USI_dataBuffer+10, tmp3
	sei
	rcall Convert_CurrentADC_to_Current
	; also reset counter
	ldi ADC_counter, 255	; Before exit counter will became 0.
	rjmp adc_exit


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
	ldi tmp, (1<<ADPS2) | (1<<ADPS1) | (0<<ADPS0) | (0<<ADIE) | (1<<ADEN)	; no interrupts
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
