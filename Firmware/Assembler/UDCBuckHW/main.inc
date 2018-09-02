; Flag PWM_flags
; bit 7 - Regulate PWM - bit sets every 1ms indicating that we can compare voltages and adjust PWM.
.EQU	PWM_flag_regulatePWM = 7

copy_buffer_to_Variables:
	sts USI_buffer_updateStatus, z0	; reset flag

	; received new data
	; we will not disable interrupts, as normally we will not receive new I2C data very soon
	; update variables from buffer
	; also check validity of data (values are in valid range)
	ldi ZL, low(USI_dataBuffer)	;Set pointer to the USI array
	ldi ZH, high(USI_dataBuffer)
	ld tmp, Z+	; Voltage_Set we update later, after Min/Max data received
	ld tmp1, Z+
	ld tmp2, Z+
	ld tmp3, Z+
	; Other data we don't care as it is read only data

	; check the validity of Voltage_Set
	; Voltage_Set should be in the range from Voltage_Min to Voltage_Max or 0.
	cp tmp, z0	; check for 0
	breq copy_buf_store_VoltSet
	cp tmp, tmp2	; check for min
	brsh copy_buf_store_VoltSet
	; We don't need to check for voltage max
	; not valid, adjust it
	clr tmp
copy_buf_store_VoltSet:
	sts Voltage_Set, tmp
	
	; check the validity of Voltage_Min
	; Voltage_Min should be smaller or equal max
	cp tmp3, tmp2
	brsh copy_buf_store_VoltMin
	mov tmp2, tmp3		; limit min
copy_buf_store_VoltMin:	
	sts Voltage_Min, tmp2

	; We don't need to check the validity of Voltage_Max as it is already in range (from previous check)
	; so, just store it
	sts Voltage_Max, tmp3

	; if Voltage_Change did not changed, then skip checks as it costs quite much CPU time
	lds tmp, Voltage_Change
	cp tmp, tmp1
	breq copy_buf_skip_VoltChange
	; check the validity of Voltage_Change
	; Voltage_Change should be greater than 3 (from 4 to 255)
	cpi tmp1, 4
	brsh copy_buf_store_VoltChange
	ldi tmp1, 4	; set min value
copy_buf_store_VoltChange:
	sts Voltage_Change, tmp1
	; now we need to convert Voltage_Change for use in timer0
	rcall Scheduler_convert_changeValue
copy_buf_skip_VoltChange:
	ret
	
; Convert ADC RAW voltage to real voltage
Convert_VoltageADC_to_Volt:
	#ifdef MOVINGAVERAGE
		ldi ZL, LOW(M_AVERAGE_voltage_COUNTER)
		ldi ZH, HIGH(M_AVERAGE_voltage_COUNTER)
		rcall moving_average
	#endif
	; Value is ready for converting to Volts (actually V*10 eg 120 = 12V)
	; The formula is: ADC * 32 / VOLT_DIV_CONST
	ldi tmp, 5
mult32:
	lsl tmp2
	rol tmp3
	dec tmp
	brne mult32
	; divide it by a constant
	mov tmpL1, tmp2
	mov tmpH1, tmp3
	ldi tmp, VOLT_DIV_CONST
	mov tmpL2, tmp
	clr tmpH2
	rcall div16u	; result in tmpH1:tmpL1 (actually only low byte)
	sts Voltage_Measured, tmpL1
	sts USI_dataBuffer+5, tmpL1
convert_V_exit:
	ret

; Convert ADC RAW current to real current
Convert_CurrentADC_to_Current:
	#ifdef MOVINGAVERAGE
		ldi ZL, LOW(M_AVERAGE_current_COUNTER)
		ldi ZH, HIGH(M_AVERAGE_current_COUNTER)
		rcall moving_average
	#endif
	; Value is ready for converting to Current (actually C*10 eg 55 = 5.5A)
	; ADC_Current_zero_RAW - reference for 0.0A
	; The formula is: (ADC-ADC_Current_zero_RAW)*64/CURRENT_DIV_CONST
	lds tmpL1, ADC_Current_zero_RAW
	lds tmpH1, ADC_Current_zero_RAW+1
	sub tmp2, tmpL1
	sbc tmp3, tmpH1
	brcc convert_C_notminus
	mov tmp2, tmpL1	; if minus, then 0
	mov tmp3, tmpH1	; if minus, then 0
convert_C_notminus:
	ldi tmp, 6
mult64:
	lsl tmp2
	rol tmp3
	dec tmp
	brne mult64
	; divide it by a constant
	mov tmpL1, tmp2
	mov tmpH1, tmp3
	ldi tmp, CURRENT_DIV_CONST
	mov tmpL2, tmp
	clr tmpH2
	rcall div16u	; result in tmpH1:tmpL1 (actually only low byte)
	sts USI_dataBuffer+6, tmpL1	; Store Current measured to buffer
convert_C_exit:
	ret
	
; The routine checks measured voltage and compare it to the desired voltage (setVolt_tmp)
; Need to see, how it is better to check voltage, by filtered RAW value or calculated V*10?
Regulate_PWM:
	; check flag PWM_flag_regulatePWM
	sbrs PWM_flags, PWM_flag_regulatePWM
	rjmp Regulate_PWM_exit
	cbr PWM_flags, (1<<PWM_flag_regulatePWM)	; clear flag
	in tmp1, OCR1A	; read current PWM
	; compare voltages
	lds tmp, Voltage_Measured	
	cp tmp, setVolt_tmp
	breq Regulate_PWM_exit	; No need to change
	brsh Regulate_PWM_minus ; need to decrease PWM
	; need to increase PWM
	; check that we not at max already
	cpi tmp1, 255
	breq Regulate_PWM_exit
	inc tmp1
Regulate_PWM_update:
	out OCR1A, tmp1
	sts USI_dataBuffer+4, tmp1	; PWM_Value
Regulate_PWM_exit:
	ret
Regulate_PWM_minus:
	; check that we not at min already
	cpi tmp1, 0
	breq Regulate_PWM_exit
	dec tmp1
	rjmp Regulate_PWM_update