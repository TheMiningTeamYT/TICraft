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
    push ix
    ld iy, 0
    add iy, sp
    lea hl, iy - 12
    ld sp, hl
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
    ; Init A
    ld a, (iy + polygonZ)
    ; Init the alternate register set
    exx
        pop de
        ex af, af'
            xor a, a
            or a, e
            jr nz, lengthNotZero
                inc a
                inc e
            lengthNotZero:
        ex af, af'
        ld c, (iy + colorOffset)
        inc c
        ld hl, (iy + texture)
        ld b, (hl)
        ld d, 16
    exx
    ; At this point, all our registers/variables should be initialized
    ; a': texture error
    ; b': texel value
    ; c': color offset
    ; d': 8
    ; e': (length of the textured line being drawn)/2
    ; hl': texture pointer
    ; a: polygonZ
    ; 75-208 cycles per pixel
    new_fillLoop:
        ; Save polygonZ to B
        ld b, a
        ; Load the texel value and advance the texture pointer
        exx ; 1
            ; Load the texel value
            ld a, b ; 2
            ; Add the color offset to the pixel
            add a, c ; 1
            ex af, af' ; 1
                sub a, d
                jr nc, textureCont
                    moveTexturePointer:
                    inc hl
                    add a, e
                    jr nc, moveTexturePointer
                    ld b, (hl)
                textureCont:
            ex af, af'
        exx ; 1
        ; Save the pixel value to C and restore polygonZ to A
        ld c, a
        ld a, b
        ; If the texel is 255 (the transparency color), skip drawing the pixel
        jr c, fill_cont ; 2/3
            dec c ; 1
            lea de, ix + 1 ; 3
            ld hl, 76799 ; 4
            add hl, de ; 1
            cp a, (hl) ; 2
            jr nc, fill_upper_cont ; 2/3
                ld (hl), a ; 2
                ld (ix), c ; 4
            fill_upper_cont:
            inc hl ; 1
            cp a, (hl) ; 2
            jr nc, fill_lower_cont ; 2/3
                ex de, hl
                ld (de), a ; 2
                ld (hl), c
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
        ; Compare e2 to dy
        ld de, (iy + dy) ; 5
        or a, a ; 1
        sbc hl, de ; 2
        ; Restore e2
        add hl, de
        ; If dy > e2, move on
        jp m, dy_cont ; 4/5
            ; Add dy to e2
            add hl, de
            add hl, de
            ; Add sx to x0
            ld de, (iy + sx) ; 5
            add ix, de ; 2
            ; Put e2 into DE and sx into HL
            ex de, hl ; 1
            add hl, bc ; 1
            ld (iy + x0), hl ; 6
            ; Check if (the previous) x0 == x1
            ld hl, (iy + x1) ; 5
            or a, a ; 1
            sbc hl, bc ; 2
            ; If x0 == x1, jump out of the loop
            jr z, real_end ; 2/3
            ; Restore e2 from DE
            ex de, hl
        dy_cont:
        ; Compare e2 to dx
        dec hl ; 1
        ld de, (iy + dx) ; 5
        or a, a ; 1
        sbc hl, de ; 2
        ; Restore e2
        inc hl
        add hl, de
        ; If e2 > dx, move on
        jp p, dx_cont ; 4/5
            ; Add dx to e2
            add hl, de
            add hl, de
            ; Save e2 to the stack
            push hl
            ; Check if y0 == y1
            ld hl, (iy + y1) ; 5
            ld bc, (iy + y0) ; 6
            or a, a ; 1
            sbc hl, bc ; 2
            ; If y0 == y1, jump out of the loop
            jr z, real_end ; 2/3
            ; Add sy to y0
            ld de, (iy + sy) ; 5
            add ix, de ; 2
            ex de, hl ; 1
            add hl, bc ; 1
            ld (iy + y0), hl ; 6
            ; Jump to the beginning of the loop
            jp new_fillLoop ; 5
        dx_cont:
        ; Push e2 back onto the stack
        push hl
        ; Jump to the beginning of the loop
        jp new_fillLoop ; 5
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