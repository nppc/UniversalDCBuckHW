;***************************************************************************
;*
;* "div16u" - 16/16 Bit Unsigned Division
;*
;* This subroutine divides the two 16-bit numbers 
;* "tmpH1:tmpL1" (dividend) and "tmpH2:tmpL2" (divisor). 
;* The result is placed in "tmpH1:tmpL1" and the remainder in
;* "tmp3:tmp2".
;*  
;* Number of words	:19+reminder routine (7)
;* Number of cycles	:235/251 (Min/Max)
;* Low registers used	:6 (tmpL1,tmpH1,tmpL2,tmpH2,tmp2,tmp3)
;* High registers used  :1 (tmp)
;*
;***************************************************************************
; about 25us at 9.6mhz
div16u:	clr	tmp2	;clear remainder Low byte
	sub	tmp3,tmp3;clear remainder High byte and carry
	ldi	tmp,17	;init loop counter
d16u_1:	rol	tmpL1		;shift left dividend
	rol	tmpH1
	dec	tmp		;decrement counter
	brne	d16u_2		;if done
	; take care of reminder (we want to have coeff as precise as possible)
	;divide divisor by 2
	lsr tmpH2
	ror tmpL2
	; compare reminder to half of divisor
	cp  tmp2, tmpL2 
	cpc	tmp3, tmpH2
	brlo d16u_exit
	; increment result
	add tmpL1, z1
	adc tmpH1, z0
d16u_exit:
	ret			;    return
d16u_2:	rol	tmp2	;shift dividend into remainder
	rol	tmp3
	sub	tmp2,tmpL2	;remainder = remainder - divisor
	sbc	tmp3,tmpH2	;
	brcc	d16u_3		;if result negative
	add	tmp2,tmpL2	;    restore remainder
	adc	tmp3,tmpH2
	clc			;    clear carry to be shifted into result
	rjmp	d16u_1		;else
d16u_3:	sec			;    set carry to be shifted into result
	rjmp	d16u_1