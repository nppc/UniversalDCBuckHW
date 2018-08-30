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