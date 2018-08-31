.EQU EEPROM_DATALEN = 3	; 3 bytes to store in EEPROM
.EQU EEPROM_STARTADDR = 5 ; it is not good to start from first byte as it can be corrupted 

.ESEG
.ORG EEPROM_STARTADDR
EEPROM_Voltage_Change:		.DB 1
EEPROM_Voltage_Min:			.DB 1
EEPROM_Voltage_Max:			.DB 1
EEPROM_CRC8:				.DB 1	; This is CRC byte

.CSEG
; reuse variables tmpL2 and tmpH2 for data and address

; Store Settings to EEPROM
; Store only changed bytes
EEPROM_StoreSettings:
	ldi tmp, EEPROM_STARTADDR
	mov tmpH2, tmp	; reuse this variable for EEPROM address
	; 1st byte
	lds tmp, Voltage_Change
	rcall ee_check_byte
	; 2nd byte
	lds tmp, Voltage_Min
	rcall ee_check_byte
	; 3nd byte
	lds tmp, Voltage_Max
	rcall ee_check_byte
	; calculate and store (if needed CRC value)
	rcall crc_calc
	cpse tmp, z0
	ret			; exit if no update needed
	dec tmpH2	; set address back to correct eeprom location for CRC
	mov tmpL2, tmp2
	cli
	rcall EEPROM_write	; Write CRC
	sei
	ret
ee_check_byte:
	rcall EEPROM_read
	cp tmpL2, tmp
	breq ee_bt_ok
	dec tmpH2		; back to correct address
	mov tmp, tmpL2
	cli
	rcall EEPROM_write
	sei
ee_bt_ok:	
	ret

; Restore data from EEPROM.
; Restore Factory settings if EEPROM is corrupted
EEPROM_restoreSettings:
	rcall crc_calc
	cpse tmp, z1
	rcall EEPROM_FactoryReset
	; EEPROM is ok - read the data
	ldi tmp, EEPROM_STARTADDR
	mov tmpH2, tmp	; reuse this variable for EEPROM address
	rcall EEPROM_read	; read byte to tmpL2. Address is incremented
	sts Voltage_Change, tmpL2
	rcall EEPROM_read	; read byte to tmpL2. Address is incremented
	sts Voltage_Min, tmpL2
	rcall EEPROM_read	; read byte to tmpL2. Address is incremented
	sts Voltage_Max, tmpL2
	ret

; This routine is called at startup, so interrupts are off.
EEPROM_FactoryReset:
	ldi tmp, EEPROM_STARTADDR
	mov tmpH2, tmp	; reuse this variable for EEPROM address
	ldi tmp, 5	; Voltage change. 5 means 0.5V/S
	mov tmpL2, tmp
	rcall EEPROM_write
	ldi tmp, 10		; Voltage min. 10 means 1V
	mov tmpL2, tmp
	rcall EEPROM_write
	ldi tmp, 120	; Voltage max. 120 means 12V
	mov tmpL2, tmp
	rcall EEPROM_write
	; calculate CRC
	rcall crc_calc
	dec tmpH2	; set address back to correct eeprom location for CRC
	mov tmpL2, tmp2
	rcall EEPROM_write	; Write CRC
	ret

;INPUT: -
;OUTPUT: tmp=1 - EEPROM ok, tmp=0 - EEPROM corrupted
; we are safe here to use T flag as it will be not corrupted in I2C routine (SREG is preserved)
crc_calc:
	ldi tmp, EEPROM_STARTADDR
	mov tmpH2, tmp	; reuse this variable for EEPROM address
	ldi tmp, EEPROM_DATALEN
	mov tmp3, tmp	; len of the data
	ldi	tmp,0x8C	; polynom
	clr	tmp2		; CRC value
crc8b:
	rcall EEPROM_read	; read byte to tmpL2. Address is incremented
	ldi	tmp1,8	; bits counter
crc01:
	eor	tmp2,tmpL2
	bst	tmp2,0
	eor	tmp2,tmpL2
	lsr	tmpL2
	lsr	tmp2
	brtc crc02
	eor	tmp2,tmp
crc02:
	dec	tmp1
	brne crc01
	dec	tmp3
	brne crc8b
	; tmp2 contains CRC value, compare it to the next EEPROM byte
	ldi tmp, 1 ; assume data is not corrupted
	rcall EEPROM_read	; read CRC byte
	cpse tmp2, tmpL2
	clr tmp		; if data is corrupted - clear flag
	ret
	

; tmpH2 - address
; tmpL2 - data
EEPROM_read:
	sbic EECR, EEPE	; wait for prev write
	rjmp EEPROM_read
	out EEARH, z0	; we will never use more than 256 bytes of EEPROM
	out EEARL, tmpH2
	sbi EECR, EERE	; start read operation
	in tmpL2, EEDR
	inc tmpH2			; next byte address
	ret
	
; tmpH2 - address
; tmpL2 - data
; every time, when calling this function, make sure interrupts are off.
EEPROM_write:
	sbic EECR,EEPE ; Wait for completion of previous write
	rjmp EEPROM_write 
	; Set Programming mode
	;ldi tmp, (0<<EEPM1) | (0<<EEPM0)
	;out EECR, tmp
	cbi EECR, EEPM1
	cbi EECR, EEPM0
	; Set up address (r18:r17) in address register
	out EEARH, z0
	out EEARL, tmpH2
	out EEDR, tmpL2
	; Start eeprom write
	sbi EECR,EEMPE
	sbi EECR,EEPE
	inc tmpH2			; next byte address
	ret

