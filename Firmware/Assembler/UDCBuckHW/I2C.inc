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
	in r_sreg, SREG
	#ifdef DEBUG
	cbi DDRB, PIN_Vsense
	#endif

	; Bounded wait for SCL to go low, or abort on STOP / timeout
	ldi itmp, 255		; Timeout counter (~223 us at 8 MHz)
USI_wait_scl:
	sbis PINB, PIN_SCL	; If SCL is Low (0), skip next instruction
	rjmp USI_scl_low	; SCL went Low -> Start condition valid!
	sbic PINB, PIN_SDA	; If SDA is High (1), do not skip (Stop condition / glitch)
	rjmp USI_start_abort	; SDA went High while SCL High -> Abort!
	dec itmp
	brne USI_wait_scl

USI_start_abort:
	; False start, glitch, STOP, or bus hang: clear flags, reset state, and exit
	clr USIstate
	ldi itmp, (1<<USISIF) | (1<<USIOIF) | (1<<USIPF)
	out USISR, itmp
	out SREG, r_sreg
	reti

USI_scl_low:
	; If previous write received data before a new/repeated START, commit it:
	sbrc USIstate, I2Cdir
	rjmp USI_start_new
	mov itmp, USIstate
	andi itmp, I2Ccounter
	cpi itmp, 2
	brlo USI_start_new
	sts USI_buffer_updateStatus, z1

USI_start_new:
	ldi USIstate, (1<<I2Cadr)	; Initialize flags for new packet

	; Enable USI Counter overflow interrupt
	in itmp,USICR
	sbr itmp, (1<<USIOIE)
	out USICR,itmp
	rcall I2C_clear_ack
	out SREG, r_sreg
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

	ldi YL, low(USI_dataBuffer)	;Set pointer to the USI array
	ldi YH, high(USI_dataBuffer)

	rcall SetACK

	sbr USIstate, (1<<I2Cack)
	cbr USIstate, I2Ccounter	; clear counter
	rjmp USI_exit


I2Cack_byte:	; Acknowledge received byte
	cbr USIstate, (1<<I2Cack)	; reset flag

	; Check if master sent NACK during read (itmp bit 0 holds the received ACK/NACK bit)
	sbrs USIstate, I2Cdir		; Skip if in Read mode (I2Cdir = 1)
	rjmp I2Cack_not_master_read

	mov itmp1, USIstate
	andi itmp1, I2Ccounter		; Check byte counter
	breq I2Cack_not_master_read	; Counter == 0: slave's ACK for address byte

	sbrc itmp, 0				; Bit 0: 0 = ACK, 1 = NACK
	rjmp I2C_data_OK			; Master sent NACK -> stop transmission and release bus!

I2Cack_not_master_read:
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
	
	; check buffer bounds before storing to prevent SRAM overflow
	cpi YL, low(USI_dataBuffer + USI_DATALEN)
	brsh I2Cbyte_skip_store
	st Y+, itmp	; store received byte
I2Cbyte_skip_store:
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
	clr USIstate
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

; Check for I2C Stop Condition (polled from main loop)
USI_stop_check:
	sbis USISR, USIPF	; Fast check: return immediately if no STOP detected
	ret

	cli					; Atomic section while reading/resetting USI state
	in tmp, USISR
	sbrs tmp, USIPF		; Re-check flag
	rjmp USI_stop_exit

	; Stop condition occurred!
	; Check if we were receiving data (Write mode):
	sbrc USIstate, I2Cdir	; If Read mode (I2Cdir = 1), no data buffer to save
	rjmp USI_stop_reset

	; Write mode (I2Cdir = 0):
	; Check if at least 1 data byte was received (counter in USIstate >= 2)
	mov tmp, USIstate
	andi tmp, I2Ccounter
	cpi tmp, 2			; counter < 2 means 0 data bytes received (empty probe)
	brlo USI_stop_reset

	; Valid data received before STOP: mark buffer as updated
	sts USI_buffer_updateStatus, z1

USI_stop_reset:
	rcall StopDetected	; clears USIstate, disables USIOIE, clears USISR, releases SDA/SCL

USI_stop_exit:
	sei
	ret
