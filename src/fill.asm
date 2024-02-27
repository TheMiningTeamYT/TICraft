section .text
; Thanks Runer112 on Cemetech!
; Source: https://www.cemetech.net/forum/viewtopic.php?t=11178&start=0
section .text
public abs
abs:
    ex de, hl ; 4
    or a, a ; 4
    sbc hl, hl ; 8
    sbc hl, de ; 8
    ret p ; 5/19
    ex de, hl ; 4
    ret ; 18
section .text
; An implementation of Bresenham's line algorithm based on the psuedo-code from Wikipedia
; https://en.wikipedia.org/wiki/Bresenham%27s_line_algorithm
public _drawTextureLineNewA_NoClip
; int startingX, int endingX, int startingY, int endingY, const uint8_t* texture, uint8_t colorOffset
_drawTextureLineNewA_NoClip:
    di
    push ix
    ld iy, 0
    add iy, sp
    lea hl, iy - 12
    ld sp, hl
    ; BC will be sx
    ld bc, 1
    ; Compute dx
    ld hl, (iy + x1)
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
    ; If x1 != x0, continue
    jr nz, zero_cont
        ; Else, check if y1 == y0
        ex de, hl
            ld hl, (iy + y1)
            ld bc, (iy + y0)
            sbc hl, bc
            ; If y1 == y0, don't render anything
            jp z, real_end
        ex de, hl
    zero_cont:
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
    ld ix, (iy + x0) ; 24
    ld de, $D40000 ; 16
    add ix, de ; 8
    ; Pre-multiply y0 & y1
    ld e, 160 ; 8
    ld h, (iy + y0) ; 16
    ld l, e ; 4
    mlt hl ; 12
    add hl, hl ; 4
    ld (iy + y0), hl ; 18
    ex de, hl ; 4
    add ix, de ; 8
    ld h, (iy + y1) ; 16
    mlt hl ; 12
    add hl, hl ; 4
    ld (iy + y1), hl ; 18
    ; Init A
    ld a, (iy + polygonZ) ; 16
    ; Init the alternate register set
    exx
        pop bc
        ld d, 16
        ex af, af'
            shiftLength:
            xor a, a
            or a, b
            jr z, lengthShiftCont
            srl d
            srl b
            rr c
            jr shiftLength
            lengthShiftCont:
            or a, c
            jr nz, lengthNotZero
                inc a
            lengthNotZero:
            ld e, a
        ex af, af'
        ld c, (iy + colorOffset)
        inc c
        ld hl, (iy + texture)
        ld b, (hl)
    exx
    ; At this point, all our registers/variables should be initialized
    ; a': texture error
    ; b': texel value
    ; c': color offset
    ; d': shifted texture length
    ; e': shifted length of the textured line being drawn
    ; hl': texture pointer
    ; a: polygonZ
    ; 75-208 cycles per pixel
    new_fillLoop:
        ; Save polygonZ to D
        ld d, a ; 4
        ; Load the texel value and advance the texture pointer
        exx ; 1
            ; Load the texel value
            ld a, b ; 4
            ; Add the color offset to the pixel
            add a, c ; 4
            ex af, af' ; 4
                sub a, d ; 4
                jr nc, textureCont ; 8/9
                    moveTexturePointer:
                    inc hl ; 4
                    add a, e ; 4
                    jr nc, moveTexturePointer ; 8/9
                    ld b, (hl) ; 9 - 208 cycles (depending on flash) (most likely: 9, 10, or 17 cycles)
                textureCont:
            ex af, af' ; 4
        exx ; 4
        ; Save the pixel value to E
        ld e, a ; 4
        ; Restore polygonZ to A
        ld a, d ; 4
        ; Load x0 into BC
        ld bc, (iy + x0) ; 24
        ; If the texel is 255 (the transparency color), skip drawing the pixel
        jr c, fill_cont ; 8/9
            dec e ; 4
            ld hl, (iy + x1) ; 23
            sbc hl, bc ; 8
            ld c, e ; 4
            lea de, ix ; 12
            ld hl, 76801 ; 16
            add hl, de ; 4
            jr z, right_fill_cont ; 8/9
                cp a, (hl) ; 8
                jr nc, right_fill_cont ; 8/9
                    ld (hl), a ; 9
                    ld (ix + 1), c ; 14
            right_fill_cont:
            dec hl ; 4
            cp a, (hl) ; 8
            jr nc, left_fill_cont ; 8/9
                ex de, hl ; 4
                ld (de), a ; 9
                ld (hl), c ; 9
            left_fill_cont:
            ld c, (iy + x0) ; 16
        fill_cont:
        ; Grab e2 from the stack
        pop hl ; 16
        ; Compare e2 to dy
        ld de, (iy + dy) ; 23
        or a, a ; 4
        sbc hl, de ; 8
        ; Restore e2
        add hl, de ; 4
        ; If dy > e2, move on
        jp m, dy_cont ; 16/17
            ; Add dy to e2
            add hl, de ; 4
            add hl, de ; 4
            ; Add sx to x0
            ld de, (iy + sx) ; 23
            add ix, de ; 8
            ; Put e2 into DE and sx into HL
            ex de, hl ; 4
            add hl, bc ; 4
            ld (iy + x0), hl ; 18
            ; Check if (the previous) x0 == x1
            ld hl, (iy + x1) ; 23
            or a, a ; 4
            sbc hl, bc ; 8
            ; If x0 == x1, jump out of the loop
            jr z, real_end ; 8/9
            ; Restore e2 from DE
            ex de, hl ; 4
        dy_cont:
        ; Compare e2 to dx
        dec hl ; 4
        ld de, (iy + dx) ; 23
        or a, a ; 4
        sbc hl, de ; 8
        ; Restore e2
        inc hl ; 4
        add hl, de ; 4
        ; If e2 > dx, move on
        jp p, dx_cont ; 16/17
            ; Add dx to e2
            add hl, de ; 4
            add hl, de ; 4
            ; Save e2 to the stack
            push hl ; 10
            ; Check if y0 == y1
            ld hl, (iy + y1) ; 23
            ld bc, (iy + y0) ; 24
            or a, a ; 4
            sbc hl, bc ; 8
            ; If y0 == y1, jump out of the loop
            jr z, real_end ; 8/9
            ; Add sy to y0
            ld de, (iy + sy) ; 23
            add ix, de ; 8
            ex de, hl ; 4
            add hl, bc ; 4
            ld (iy + y0), hl ; 18
            ; Jump to the beginning of the loop
            jr new_fillLoop ; 9
        dx_cont:
        ; Push e2 back onto the stack
        push hl ; 10
        ; Jump to the beginning of the loop
        jp new_fillLoop ; 17
    real_end:
    ld sp, iy
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
    ; Carry flag is always set by the time we get here, so this sets HL to -1
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
        ld a, (hl) ; 8
        cp a, c ; 4
        jr nc, shadeCont ; 8/9
            add a, c ; 4
            ld (hl), a ; 8
        shadeCont:
        inc hl ; 4
        sbc hl, de ; 8
        add hl, de ; 4
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
x0 = 6
private x1
x1 = 9
private y0
y0 = 12
private y1
y1 = 15
private texture
texture = 18
private colorOffset
colorOffset = 21
private polygonZ
polygonZ = 24
private dx
dx = -3
private sx
sx = -6
private dy
dy = -9
private sy
sy = -12
extern _texPalette