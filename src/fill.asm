section .text
assume adl = 1
; stole some code from people on a forum
section .text
public abs
abs:
    ex de, hl ; 1
    or a, a ; 1
    sbc hl, hl ; 2
    sbc hl, de ; 2
    ret p ; 2/6
    ex de, hl ; 1
    ret ; 6
section .text
public _drawTextureLineA
; Fixed24 startingX, Fixed24 endingX, Fixed24 startingY, Fixed24 endingY, const uint8_t* texture, uint8_t colorOffset
; definitely has room for optimization
; probably could be optimized using Bresenham's line algorithm (or the line algorithm used by graphx ... as soon as I figure out how it works)
_drawTextureLineA:
    di
    ; x1, y1, xStep, yStep, ratio, and column are Fixed24's (the numbers are effectively multiplied by 4096) FYI
    ; save registers
    push ix
    push iy
    ; MUHAHA iy IS the stack pointer now!
    ld iy, 9
    add iy, sp
    ld ix, x1 
    ; grab arguments off the stack
    ; save stack pointer
    ld (startingSP),sp
    ld bc, (iy)
    ld (ix), bc
    ld hl, (iy + 3)
    or a, a
    ; dx is now in hl
    sbc hl, bc
    ; dx is now in de
    ex de, hl
    ld bc, (iy + 6)
    ld (ix + 3), bc
    ld hl, (iy + 9)
    or a, a
    ; dy is now in hl
    sbc hl, bc
    ; push dx & dy to the stack
    push hl ; dy
    push de ; dx
    ; grab absolute value of dx & dy
    ex de, hl
    ; hl: dx, de: dy
    call abs
    push hl ; save abs(dx) to the stack
    ld hl, (iy - 12)
    ; hl: dy, de: garbage
    call abs
    ; hl: abs(dy) de: garbage
    ex de, hl
    ld hl, (iy - 18)
    ; hl: abs(dx), de: abs(dy)
    or a, a
    sbc hl, de
    pop hl
    jr nc, cont_length

    y_is_greater:
    ex de, hl

    cont_length:
    ; length is in hl
    ld de, 4096
    add hl, de
    push hl
    push de
    ld l, (iy - 16) ; upper bits in l
    ld a, h ; middle bits in a
    srl l
    rra
    srl l
    rra
    srl l
    rra
    srl l
    rra
    ex de, hl
    ld h, e
    ld l, a
    ld (ix + 21), hl
    call _fp_div
    ; the reciprocal of the length is now in hl
    inc sp
    inc sp
    inc sp
    ld (iy - 18), hl
    ; the reciprocal of the length is now in bc
    exx
    ld bc, (iy - 18)
    exx
    call _fp_mul
    ld (ix + 6), hl
    inc sp
    inc sp
    inc sp
    exx
    ld (iy - 15), bc
    exx
    call _fp_mul
    ld (ix + 9), hl
    exx
    or a, a
    sbc hl, hl
    ; store 0 into column
    ld (ix + 12), hl
    add hl, bc
    add hl, hl
    add hl, hl
    add hl, hl
    add hl, hl
    push hl
    exx
    ld hl, (iy + 12)
    ld (ix + 18), hl
    ld a, (iy + 15)
    pop bc
    exx
    ld hl, (ix + 21)
    ld bc, 1
    ld d, a
    ld e, 255
    exx
    ld iy, x1 
    ; 165-182 cycles -- optimization successful
    fillLoop:
        ; ix: screen pointer
        ; iy: variable pointer

        ; 5 cycles
        ld ix, gfx_vram + 127 ; 5

        ; 11 cyles
        ; move on X
        ld de, (iy) ; 5
        ld hl, (iy + 6) ; 5
        add hl, de ; 1

        ; 10/13 cycles
        ; make sure that x isn't 320 or greater, else cap it to 319
        ex de, hl ; 1
        ld hl, 1310719 ; 4
        sbc hl, de ; 2
        jr nc, x_1 ; 2/3
        ld de, 1306624 ; 4

        x_1:
        ; 29 cycles
        ld (iy), de ; 6
        ld e, (iy + 2) ; 4 // upper bits in e
        ld a, d ; 1 // middle bits in a
        ; bit shifting
        srl e ; 2
        rra ; 1
        srl e ; 2
        rra ; 1
        srl e ; 2
        rra ; 1
        srl e ; 2
        rra ; 1
        or a, a ; 1
        sbc hl, hl ; 2
        ; effectively shift it right 8 by putting the upper 8 bits (e) into the middle 8 bits of hl (h)
        ; and the middle 8 bits (a) into the lower 8 bits of hl (l)
        ld h,e ; 1
        ld l,a ; 1
        ex de, hl ; 1

        ; 2 cycles
        ; Set ix
        add ix, de ; 2

        ; 11 cycles
        ; move on Y
        ld de, (iy + 3) ; 5
        ld hl, (iy + 9) ; 5
        add hl,de ; 1

        ; 10/13 cycles
        ; check that y is less than 239, else cap it to 238
        ex de, hl ; 1
        ld hl, 978944 ; 4
        sbc hl, de ; 2
        jr nc, y_1 ; 2/3
        ld de, 974848 ; 4

        ; 28 cycles
        y_1:
        ld (iy + 3), de ; 6
        ld e, (iy + 5) ; 4 // upper bits in e
        ld a, d ; 1 // middle bits in a
        ; bit shifting
        srl e ; 2
        rra ; 1
        srl e ; 2
        rra ; 1
        srl e ; 2
        rra ; 1
        srl e ; 2
        rra ; 1
        or a, a ; 1
        sbc hl, hl ; 2
        ; effectively shift it right 8 by putting the upper 8 bits (e) into the middle 8 bits of hl (h)
        ; and the middle 8 bits (a) into the lower 8 bits of hl (l)
        ld l,a ; 1


        ; 11 cycles
        ; Set IY
        ; Multiply HL by 320
        ; since hl (the y value) should be less than 256 (or 240),
        ; we can discard the middle and upper 8 bits and use mlt
        ; muliply l by 160
        ld h, 160 ; 2
        mlt hl ; 6
        ; add hl to itself (multiply by 2)
        add hl,hl ; 1
        ex de, hl ; 1
        ; add this value to the screen pointer
        add ix, de ; 2

        ; Get column
        ; 22 cycles
        ld a, (iy + 13) ; 4
        srl a ; 2
        srl a ; 2
        srl a ; 2
        srl a ; 2
        ld de, (iy + 18) ; 5
        or a, a ; 1
        sbc hl, hl ; 2
        ld l, a ; 1
        add hl, de ; 1

        ; At this point we should have a pointer to the texture in hl and a pointer to the screen location in IX
        ; Load pixel
        ; 2 cycles
        ld a, (hl) ; 2

        ; advance column
        ; 12 cycles
        ld hl, (iy + 12) ; 5
        ; texture ratio is in bc
        add hl, bc ; 1
        ld (iy + 12), hl ; 6

        ; Copy pixel (continuted)
        ; 5/16 cycles
        exx ; 1
        cp a, e ; 1
        jr z, endOfLoop ; 2/3
        add a, d ; 1
        ld (ix-127), a ; 4
        lea ix, ix+127 ; 3
        ld (ix+66), a ; 4

        ; 8 cycles
        endOfLoop:
        sbc hl, bc ; 2
        exx ; 1
        jp p, fillLoop ; 5
    ; the end:
    ld sp,(startingSP)
    pop iy
    pop ix
    ei
    ret
section .text
public _int_sqrt_a
; unsigned int n
; returns in hl
_int_sqrt_a:
    di
    push iy
    ld iy, 0
    add iy, sp

    ; number we are getting sqrt of is in de
    ld de, (iy + 6)
    or a, a
    sbc hl, hl
    inc hl
    sbc hl, de
    ; if number is 0 or 1, the square root is no different
    jr nc, zero_or_one

    ; get number of bits
    or a, a
    ld a, 24
    ld hl, $7FFFFF
    sbc hl, de
    ; if the number already has 1 in the first position, move on
    jr c, count_cont ; 3
    count_loop:
        add hl, de ; 1
        dec a ; 1
        ex de, hl ; 1
        add hl, hl ; 1
        ex de, hl ; 1
        sbc hl, de ; 2
        jr nc, count_loop ; 3
    count_cont:
    ld b, a
    and a, 1
    add a, b
    exx
    or a, a
    sbc hl, hl
    ex de, hl
    sbc hl, hl
    exx
    ; result is in de'
    ; result squared is in hl'
    ; n will be in hl
    ; worst case scenario for 1 loop is 305 cycles -- not bad, not great
    sqrt_loop:
        sub a, 2 ; 2
        exx ; 1
        ex de, hl ; 1
        add hl, hl ; 1
        ex de, hl ; 1
        add hl, hl ; 1
        add hl, hl ; 1
        add hl, de ; 1
        inc de ; 1
        add hl, de ; 1
        push hl ; 4
        exx ; 1
        ld hl, (iy + 6) ; 6
        or a, a ; 1
        jr z, shift_cont ; 3
        ld c, (iy + 8) ; 4
        ld b, a ; 1
        ; 10 cycles -- nice
        shift_loop:
            sra c ; 2
            rr h ; 2
            rr l ; 2
            djnz shift_loop ; 4
        push hl ; 4
        ld (iy - 4), c ; 4
        pop hl ; 4
        ; at this point, shifted n is in hl
        shift_cont:
        pop de ; 4
        ; result squared is now in de
        or a, a ; 1
        sbc hl, de ; 2
        jr nc, sqrt_loop_end ; 3
            exx ; 1
            or a, a ; 1
            sbc hl, de ; 2
            dec de ; 1
            sbc hl, de ; 2
            exx ; 1
        sqrt_loop_end:
        or a, a ; 1
        jr nz, sqrt_loop ; 3
    exx
    push de
    exx
    pop de
    zero_or_one:
    ex de, hl
    the_end:
    ld sp, iy
    pop iy
    ei
    ret
section .data
private gfx_vram
gfx_vram = $D40000
private x1
x1: db 33 dup 00h
private startingSP
startingSP: db 3 dup 00h
extern _fp_div
extern _fp_mul
extern _cameraXYZ
extern _sx
extern _cx
extern _sy
extern _cy
extern _sz
extern _cz