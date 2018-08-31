#ifdef MOVINGAVERAGE

; while Mov Average init interrupts should be disabled
init_Moving_Average:
	cli	; just in case
	sts M_AVERAGE_voltage_COUNTER, z0
	sts M_AVERAGE_current_COUNTER, z0
	ret


; smooth RC signal via moving average algorithm
; store/read 10 bit value to the Moving Average array
; value is in tmp2:tmp3 
; Z points to the array of data (Counter and averaging data)
moving_average:
	; we will run average within MOVINGAVERAGE_N values.
	; pointer for moving average table
	push ZL	; store for future use
	push ZH	; store for future use
	ld tmp, Z+
	mov tmp1, tmp
	lsl tmp1	; multiply by 2 because we operate with 16 bit values
	add ZL, tmp1
	adc ZH, z0
	; store current value in the table
	st Z+, tmp2
	st Z, tmp3
	; update counter
	inc tmp
	cpi tmp, MOVINGAVERAGE_N
	brne mov_avrg_skp
	clr tmp
mov_avrg_skp:
	pop ZH
	pop ZL
	st Z+, tmp	; Store counter
	; sum all values in the table
	; Z points to the first element in the table
	ldi tmp, MOVINGAVERAGE_N
	clr tmpL1
	clr tmpH1
mov_avrg_loop:
	ld tmp2, Z+
	ld tmp3, Z+
	add tmpL1, tmp2
	adc tmpH1, tmp3
	dec tmp
	brne mov_avrg_loop
	; divide result by MOVINGAVERAGE_N
	ldi tmp, MOVINGAVERAGE_N
	mov tmpL2, tmp
	clr tmpH2
	rcall div16u
	; new result store back to tmp2:tmp3
	mov tmp2, tmpL1
	mov tmp3, tmpH1
	ret

#endif	
