assume adl=1
section .text
; approximates the square root of num
; 2^(floor((floor(log2(num))+1)/2)-1) + (num)/(2^(floor((floor(log2(num))+1)/2)+1))
; Seems to be consistently within ±6% of the real answer
public _approx_sqrt_a
_approx_sqrt_a:
    ; Set hl to 3
    ld hl, 3
    add hl, sp
    ld de, (hl)
    sbc hl, sp
    sbc hl, de
    ex de, hl
    ; If the number is greater than 3, move on
    jr c, threeCont
    ; Else, if the number is 1 or 0, return HL
    bit 1, l
    ret z
    ; Else, return HL - 1
    dec hl
    ret
threeCont:
    ; Save the original number for later
    push hl
    ; Count the number of bits
    ; ((x+1)/2)
    ld b, 12
    add hl, hl
    jr c, count_bits_cont
    dec b
    add hl, hl
    jr c, count_bits_cont
    add hl, hl
    jr c, count_bits_cont
    dec b
    add hl, hl
    jr c, count_bits_cont
    add hl, hl
    jr c, count_bits_cont
    dec b
    add hl, hl
    jr c, count_bits_cont
    add hl, hl
    jr c, count_bits_cont
    dec b
    add hl, hl
    jr c, count_bits_cont
    add hl, hl
    jr c, count_bits_cont
    dec b
    add hl, hl
    jr c, count_bits_cont
    add hl, hl
    jr c, count_bits_cont
    dec b
    add hl, hl
    jr c, count_bits_cont
    add hl, hl
    jr c, count_bits_cont
    dec b
    add hl, hl
    jr c, count_bits_cont
    add hl, hl
    jr c, count_bits_cont
    dec b
    add hl, hl
    jr c, count_bits_cont
    add hl, hl
    jr c, count_bits_cont
    dec b
    add hl, hl
    jr c, count_bits_cont
    add hl, hl
    jr c, count_bits_cont
    dec b
    add hl, hl
    jr c, count_bits_cont
    add hl, hl
    jr c, count_bits_cont
    dec b
    count_bits_cont:
    ; generate our guess
    ld a, b
    sub a, 9
    ; If the log is less than 9 (true log is less than 18), continue
    jr c, log_less_than_9
        di
        ; Else, shift the number right by 8 and the guess left by 8
        ld b, a
        inc sp
        pop de
        dec sp
        ld a, e
        ld hl, 256
        jr nz, shift_guess
        srl d
        rra
        srl d
        rra
        ld e, a
        
        ; add the guess and the shifted original number together
        add hl, de
        ret
    log_less_than_9:
    pop de
    ; Because the carry flag is always set by the time we get here, this gives -1
    sbc hl, hl
    add hl, sp
    ; Shift the original number right by 1 to start
    srl (hl)
    ; Start our guess at 2
    ld hl, 2
    dec b
    ; If the log is 1, return
    ret z
    ; Finish shifting the original number right
    ld a, e
    rr d
    rra
    dec b
    jr z, guess_cont
    ; 1 << b
    ; (2^b)
    shift_guess:
    ; Shift our guess left and the original number right
    add hl, hl
    srl d
    rra
    dec b
    jr z, guess_cont
    add hl, hl
    srl d
    rra
    dec b
    jr z, guess_cont
    add hl, hl
    srl d
    rra
    dec b
    jr z, guess_cont
    add hl, hl
    srl d
    rra
    dec b
    jr z, guess_cont
    add hl, hl
    srl d
    rra
    dec b
    jr z, guess_cont
    add hl, hl
    srl d
    rra
    guess_cont:
    ; Shift the original number right 2 final times
    srl d
    rra
    srl d
    rra
    ld e, a
    ; add the guess and the shifted original number together
    add hl, de
    ret