; USIstate (4 bits flags and counter bits 0-3):
.equ	I2Cadr			= 7	; wait for address
.equ	I2Cack			= 6	; acknowledge received byte
.equ	I2Cdir			= 5	; Read=0, Transmit=1
.equ	I2Cbyte			= 4	; Process byte according to I2Cdir
.equ	I2Ccounter		= 0b00001111	; Counter bits (0-3)
; remaining bits is counter for send/received bytes

; called from setup (not onterrupts)
init_USI:
	ldi tmp, (1<<USISIE) | (1<<USIWM1) | (1<<USIWM0) | (1<<USICS1)
	out USICR, tmp
	ldi tmp, (1<<USISIF) | (1<<USIOIF) | (1<<USIPF)
	out USISR, tmp

	; start with all pins floated (pulled up by external resistors)
	sbi PORTB, PIN_SCL	; should be 1. If Atmel doc says different - don't trust it. :)
	sbi PORTB, PIN_SDA	; should be always 1.
	sbi DDRB, PIN_SCL	; should be 1. If Atmel doc says different - don't trust it. :)
	cbi DDRB, PIN_SDA 
	
	clr USIstate	; clear flags and counter
	
	; We also need to update some bytes in buffer that we restored from EEPORM
	sts USI_dataBuffer, z0	; Voltage_Set always 0 at the beginning
	lds tmp, Voltage_Change
	sts USI_dataBuffer+1,tmp	; Voltage_Change
	lds tmp, Voltage_Min
	sts USI_dataBuffer+2,tmp	; Voltage_Min
	lds tmp, Voltage_Max
	sts USI_dataBuffer+3,tmp	;Voltage_Max
	ret
	
; Other routines called from interrupts
	
USI_start:
	; SREG register is not affected here
	#ifdef DEBUG
	cbi DDRB, PIN_Vsense
	#endif

	sbic PINB,PIN_SCL	;Wait for SCL to go low to ensure the "Start Condition" has completed.
	rjmp PC-1			;Without this, I2c is unstable.

	ldi USIstate, (1<<I2Cadr)	; Initialize flags for new packet

	; Enable USI Counter overflow interrupt
	in itmp,USICR
	sbr itmp, (1<<USIOIE)
	out USICR,itmp
	rcall I2C_clear_ack
	reti

USI_ovf:
	in r_sreg, SREG

	in itmp, USIDR	; read data from buffer
	out USIDR, z0	; clear data register to be prepared for ack.
	
	; Lets check why we came here (Is this is address or a data or something else)
	sbrc USIstate, I2Cadr
	rjmp I2Cadr_check
	sbrc USIstate, I2Cack
	rjmp I2Cack_byte
	sbrc USIstate, I2Cbyte
	rjmp I2Cbyte_process

	rjmp USI_exit

I2Cadr_check:
	; we already have in itmp address and data direction bit
	bst itmp,0	; store data direction bit
	andi itmp,  0xFE	; reset data direction bit
	cpi itmp, (USI_ADDRESS<<1)
	brne I2Cbadadr
	cbr USIstate, (1<<I2Cadr)	; Address processed - clear flag
	bld USIstate, I2Cdir	; set flag according to data direction bit
	
	#ifdef DEBUG
		sbi DDRB, PIN_Vsense	; Address OK
	#endif

	; Now is good time to update data in buffer
	; If master requested the data, update buffer from variables
	;sbrs USIstate, I2Cdir
	;rjmp I2Cadr_skip_copybuf
	;ldi YL, low(USI_dataBuffer + 4)	;Set pointer to the USI array. We don't need to update first 4 bytes as they are changing at startup or after I2C data received.
	;ldi YH, high(USI_dataBuffer + 4)
	;lds itmp, PWM_Value
	;st Y+, itmp
	;lds itmp, Voltage_Measured
	;st Y+, itmp
	;lds itmp, Current_Measured
	;st Y+, itmp
	;lds itmp, ADC_Voltage_RAW
	;st Y+, itmp
	;lds itmp, ADC_Voltage_RAW+1
	;st Y+, itmp
	;lds itmp, ADC_Current_RAW
	;st Y+, itmp
	;lds itmp, ADC_Current_RAW+1
	;st Y+, itmp

I2Cadr_skip_copybuf:	
	ldi YL, low(USI_dataBuffer)	;Set pointer to the USI array
	ldi YH, high(USI_dataBuffer)

	rcall SetACK

	sbr USIstate, (1<<I2Cack)
	cbr USIstate, I2Ccounter	; clear counter
	rjmp USI_exit


I2Cack_byte:	; Acknowledge received byte
	cbr USIstate, (1<<I2Cack)	; reset flag
	rcall I2C_clear_ack
	mov itmp, USIstate	
	inc itmp	; increment counter. Flags bits will be not touched
	andi itmp, I2Ccounter	; we need to filter counter bits
	cpi itmp, USI_DATALEN+1	; total bytes to receive (including address byte)
	breq I2C_data_OK
	; store new counter value
	cbr USIstate, I2Ccounter	; erase counter bits
	or USIstate, itmp			; store counter bits
	
	sbr USIstate, (1<<I2Cbyte)	; Receive/Send next byte
	; if we transmit the data, then fill USI data register
	sbrs USIstate, I2Cdir
	rjmp USI_exit
	
	sbi	DDRB, PIN_SDA	; Seems this is important	
	ld itmp, Y+ ; load data from array
	out USIDR, itmp
	
	rjmp USI_exit

I2C_data_OK:	; Received/sent all bytes

	#ifdef DEBUG
		cbi DDRB, PIN_Vsense
	#endif

	; If master sent new data, then make it known to main loop for processing
	sbrs USIstate, I2Cdir
	sts USI_buffer_updateStatus, z1	; New data arrived

I2Cbadadr:
	rcall StopDetected

USI_exit:
	out SREG, r_sreg
	reti

I2Cbyte_process:
	; check, does we read or transmit data?
	sbrc USIstate, I2Cdir
	rjmp I2Cbyte_process_transmit
	
	; we already have data in itmp
	st Y+, itmp	; store received byte
	rcall SetACK

	sbr USIstate, (1<<I2Cack)
	rjmp USI_exit

I2Cbyte_process_transmit:
	; need to wait for ACK from master
	cbi	DDRB, PIN_SDA	; release the line to give master possibility to answer
	; wait for ack bit
	ldi itmp,(1<<USIOIF) | (1<<USIPF) | (1<<USIDC) | (0x0E<<USICNT0)
	out USISR, itmp
	sbr USIstate, (1<<I2Cack)
	rjmp USI_exit

	
SetACK:
	sbi	DDRB, PIN_SDA
	sbi PortB, PIN_SCL
	sbi DDRB, PIN_SCL

	ldi itmp,(1<<USIOIF) | (1<<USIPF) | (1<<USIDC) | (0x0E<<USICNT0)
	out USISR, itmp
	ret


StopDetected:

	in itmp, USICR;(Disable USI Counter overflow interrupt)
	cbr itmp, 0b01000000
	out USICR, itmp

I2C_clear_ack:

	cbi	DDRB, PIN_SDA
	cbi PortB, PIN_SCL
	cbi DDRB, PIN_SCL

	ldi itmp, (1<<USISIF) | (1<<USIOIF) | (1<<USIPF)
	out USISR, itmp
	ret
