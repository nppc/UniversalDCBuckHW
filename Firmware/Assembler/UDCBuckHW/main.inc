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
	; Convert?
	sbrs ADC_flags, flag_ADCvoltageReady
	rjmp convert_V_exit
	; reset flag 
	cbr ADC_flags, (1<<flag_ADCvoltageReady)
	lds tmp2, ADC_Voltage_RAW
	lds tmp3, ADC_Voltage_RAW+1
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
convert_V_exit:
	ret


; Convert ADC RAW current to real current
Convert_CurrentADC_to_Current:
	; Convert?
	sbrs ADC_flags, flag_ADCcurrentReady
	rjmp convert_C_exit
	; reset flag 
	cbr ADC_flags, (1<<flag_ADCcurrentReady)
	lds tmp2, ADC_Current_RAW
	lds tmp3, ADC_Current_RAW+1
	#ifdef MOVINGAVERAGE
		ldi ZL, LOW(M_AVERAGE_current_COUNTER)
		ldi ZH, HIGH(M_AVERAGE_current_COUNTER)
		rcall moving_average
	#endif
	; Value is ready for converting to Current (actually C*10 eg 55 = 5.5A)
	; ADC_Current_zero_RAW - reference for 0.0A
	; TODO
convert_C_exit:
	ret