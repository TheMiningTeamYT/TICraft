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
public _drawTextureLineNewA_NoClip
; int startingX, int endingX, int startingY, int endingY, const uint8_t* texture, uint8_t colorOffset
_drawTextureLineNewA_NoClip:
    di
    ld iy, 0
    add iy, sp
    push ix
    ; BC will be sx
    ld bc, 1
    ; Compute dx
    ld hl, (iy + x1)
    dec hl
    ld (iy + x1), hl
    ld de, (iy + x0)
    or a, a
    sbc hl, de
    ; If x1 >= x0, continue
    jr nc, sx_cont
        ; Else, set sx (bc) to -1
        dec bc
        dec bc
    sx_cont:
    ; Store sx
    ld (iy + sx), bc
    ; Compute abs(dx)
    call abs
    ; Store dx
    ld (iy + dx), hl
    ; BC will be sy
    ld bc, 320
    ; Compute dy
    ld hl, (iy + y1)
    ld de, (iy + y0)
    or a, a
    sbc hl, de
    ; if y1 >= y0, continue
    jr nc, sy_cont
        ; Else, set sy (bc) to -320
        ld bc, -320
    sy_cont:
    ; Store sy
    ld (iy + sy), bc
    ; Compute abs(dy)
    call abs
    ; save abs(dy) to the stack
    push hl
    ; Negate hl
    ex de, hl
    or a, a
    sbc hl, hl
    sbc hl, de
    ; Store hl to dy
    ld (iy + dy), hl
    ; Load dx
    ld de, (iy + dx)
    add hl, de
    add hl, hl
    ; Push error*2 to the stack and retrieve abs(dy) from the stack
    ex (sp), hl
    or a, a
    sbc hl, de
    ex de, hl
    ; Jump if dx is greater
    jr c, textureRatio_cont
        ; Else, restore dy
        add hl, de
    textureRatio_cont:
    ; At this point dx or dy (whichever has a bigger absolute value) is in hl
    push hl ; 4
    ; 52 cycles
    ; offset ix (screen pointer) by gfx_vram
    ld ix, (iy + x0) ; 6
    ld de, $D40000 ; 4
    add ix, de ; 2
    ; Pre-multiply y0 & y1
    ld e, 160 ; 2
    ld h, (iy + y0) ; 4
    ld l, e ; 1
    mlt hl ; 6
    add hl, hl ; 1
    ld (iy + y0), hl; 6
    ex de, hl ; 1
    add ix, de ; 2
    ld h, (iy + y1) ; 4
    mlt hl ; 6
    add hl, hl ; 1
    ld (iy + y1), hl ; 6

    ; Init column & texture ratio & draw first pixel
    exx
        pop de
        xor a, a
        srl d
        rr e
        rra
        srl e
        rra
        srl e
        rra
        srl e
        rra
        ld d, a
        ld b, a
        or a, e
        jr nz, not_zero
            inc e
        not_zero:
        ld c, (iy + colorOffset)
        inc c
        ld hl, (iy + texture)
        ex af, af'
            ld a, e
        ex af, af'
    exx
    ; At this point, all our registers/variables should be initialized
    ; hl': texture
    ; c': colorOffset
    ; b': running error
    ; e': whole number component of length / 16
    ; d': fractional component of length / 16
    ; a': Running total of the fractional component of column
    ; 75-208 cycles per pixel
    new_fillLoop:
        ; Load the texel value and advance column
        exx ; 1
            ; Add the color offset to the pixel
            ld a, (hl) ; 2
            add a, c ; 1
            ex af, af' ; 1
                or a, a ;1
                jr nz, decrement_texture_error ; 2/3
                    move_pointer:
                    ld a, b ; 1
                    add a, d ; 1
                    ld b, a ; 1
                    ld a, e ; 1
                    inc hl ; 1
                    adc a, 0 ; 2
                    jr z, move_pointer ; 2/3
                decrement_texture_error:
                dec a ; 1
            ex af, af'
        exx ; 1
        ; If the texel is 255 (the transparency color), skip drawing the pixel
        jr c, fill_cont ; 2/3
            dec a ; 1
            lea hl, ix ; 3
            ld de, 76800 ; 4
            add hl, de ; 1
            ld c, a ; 1
            ld a, (iy + polygonZ) ; 4
            cp a, (hl) ; 2
            jr nc, fill_upper_cont ; 2/3
                ld (hl), a ; 2
                ld (ix), c ; 4
            fill_upper_cont:
            inc hl ; 1
            cp a, (hl) ; 2
            jr nc, fill_lower_cont ; 2/3
                ld (hl), a ; 2
                ld (ix + 1), c ; 4
        fill_cont:
        or a, a ; 1
        fill_lower_cont:
        ; Test if x0 == x1
        ld hl, (iy + x1) ; 5
        ld bc, (iy + x0) ; 6
        sbc hl, bc ; 2
        ; If x0 != x1, move on
        jr nz, end_cont ; 2/3
            ; Else, test if y0 == y1
            ld hl, (iy + y0) ; 5
            ld de, (iy + y1) ; 5
            sbc hl, de ; 2
            ; If y0 == y1 as well, jump out of the loop
            jr z, real_end ; 2/3
        end_cont:
        ; Grab e2 from the stack
        pop hl ; 4
        dec sp ; 1
        dec sp ; 1
        dec sp ; 1
        ; Compare e2 to dy
        ld de, (iy + dy) ; 5
        or a, a ; 1
        sbc hl, de ; 2
        ; If dy > e2, move on
        jp m, dy_cont ; 4/5
            ; Check if x0 == x1
            ld hl, (iy + x1) ; 5
            or a, a ; 1
            sbc hl, bc ; 2
            ; If x0 == x1, jump out of the loop
            jr z, real_end ; 2/3
            ; Else, add dy to error
            pop hl ; 4
            add hl, de ; 1
            add hl, de ; 1
            push hl ; 4
            ; Add sx to x0
            ld de, (iy + sx) ; 5
            add ix, de ; 2
            ex de, hl ; 1
            add hl, bc ; 1
            ld (iy + x0), hl ; 6
        dy_cont:
        ; Grab e2 from the stack
        pop hl ; 4
        dec sp ; 1
        dec sp ; 1
        dec sp ; 1
        dec hl ; 1
        ; Compare e2 to dx
        ld de, (iy + dx) ; 5
        or a, a ; 1
        sbc hl, de ; 2
        ; If e2 > dx, move on
        jp p, dx_cont ; 4/5
            ; Check if y0 == y1
            ld hl, (iy + y1) ; 5
            ld bc, (iy + y0) ; 6
            or a, a ; 1
            sbc hl, bc ; 2
            ; If y0 == y1, jump out of the loop
            jr z, real_end ; 2/3
            ; Else, add dx to error
            pop hl ; 4
            add hl, de ; 1
            add hl, de ; 1
            push hl ; 4
            ; Add sy to y0
            ld de, (iy + sy) ; 5
            add ix, de ; 2
            ex de, hl ; 1
            add hl, bc ; 1
            ld (iy + y0), hl ; 6
        dx_cont:
        ; Jump to the beginning of the loop
        jp new_fillLoop ; 5
    real_end:
    inc sp
    inc sp
    inc sp
    pop ix
    ei
    ret
section .text
public _approx_sqrt_a
public approx_sqrt_a
_approx_sqrt_a:
    ld hl, 3
    add hl, sp
    ld hl, (hl)
; approximates the square root of hl
; 2^(floor((floor(log2(hl))+1)/2)-1) + (hl)/(2^(floor((floor(log2(hl))+1)/2)+1))
approx_sqrt_a:
    ex de, hl
    ; clear the carry flag and set hl to 1
    or a, a
    sbc hl, hl
    inc hl
    sbc hl, de
    ex de, hl
    ; if the number is 0 or 1, the square root is no different
    ret nc
    ; we need to save the value in hl for later
    push hl
    ld b, 11
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
    sub a, 8
    jr c, log_less_than_8
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
        
        ; add the guess and the divided square together
        add hl, de
        ret
    log_less_than_8:
    pop de
    sbc hl, hl
    add hl, sp
    ld a, e
    ld e, (hl)
    srl e
    rr d
    rra
    ld hl, 2
    dec b
    jr z, guess_cont
    ; 1 << b
    ; (2^b)
    shift_guess:
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
    srl d
    rra
    srl d
    rra
    ld e, a

    ; add the guess and the divided square together
    add hl, de
    ret
section .text
public _shadeScreen
_shadeScreen:
    ld hl, gfx_vram
    ld de, gfx_vram + 76800
    ld bc, 76800
    ldir
    ld c, 126
    shadeLoop:
        ld a, (hl) ; 2
        cp a, c ; 1
        jr nc, shadeCont ; 2/3
            add a, c ; 1
            ld (hl), a ; 2
        shadeCont:
        inc hl ; 1
        sbc hl, de ; 2
        add hl, de ; 1
        jr nz, shadeLoop ; 2/3
    ; right now hl & de point to the end of the back buffer
    dec hl
    ld de, gfx_vram + 76799
    ld bc, 76800
    lddr
    ld hl, (_texPalette)
    ld bc, 512
    add hl, bc
    ld bc, (hl)
    ld hl, $E303FE
    ld (hl), c
    inc hl
    ld (hl), b
    ret
section .data
private gfx_vram
gfx_vram = $D40000
extern _fp_div
extern _fp_mul
extern __idvrmu
private x0
x0 = 3
private x1
x1 = 6
private y0
y0 = 9
private y1
y1 = 12
private texture
texture = 15
private colorOffset
colorOffset = 18
private polygonZ
polygonZ = 21
private dx
dx = -24
private sx
sx = -27
private dy
dy = -30
private sy
sy = -33
private error
error = -36
extern _texPalette