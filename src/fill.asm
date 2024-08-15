assume adl=1
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
    ; Negate hl
    ex de, hl
    ; Store hl to dy
    ld (iy + dy), hl
    ; Load dx
    ld bc, (iy + dx)
    add hl, bc
    add hl, hl
    ; Put error*2 into DE and abs(dy) into HL
    ex de, hl
    or a, a
    sbc hl, bc
    ; Push dx to the stack
    push bc
    ; Jump if dx is greater
    jr c, textureRatio_cont
        ; Else, restore dy and push it to the stack
        add hl, bc
        ex (sp), hl
    textureRatio_cont:
    ; 52 cycles
    ; offset ix (screen pointer) by gfx_vram
    ld ix, (iy + x0) ; 24
    ld bc, $D40000 ; 16
    add ix, bc ; 8
    ; Pre-multiply y0 & y1
    ld h, (iy + y0) ; 16
    ld l, 160 ; 8
    mlt hl ; 12
    add hl, hl ; 4
    ld (iy + y0), hl ; 18
    ex de, hl ; 4
    add ix, de ; 8
    ex de, hl ; 4
    ld h, (iy + y1) ; 16
    ld l, 160 ; 8
    mlt hl ; 12
    add hl, hl ; 4
    ld (iy + y1), hl ; 18
    ; Modify the self-modifying code
    ld a, (iy + polygonZ)
    ld (selfModifyingCode1 + 1), a
    ld (selfModifyingCode2 + 1), a
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
    exx ; 4
    ; At this point, all our registers/variables should be initialized
    ; a': texture error
    ; b': texel value
    ; c': color offset
    ; d': shifted texture length
    ; e': shifted length of the textured line being drawn
    ; hl': texture pointer
    ; 75-208 cycles per pixel
    new_fillLoop:
        ; Load the texel value and advance the texture pointer
        exx ; 4
        new_fillLoop_entry_point:
            ; Load the texel value
            ld a, b ; 4
            ; Add the color offset to the pixel
            add a, c ; 4
            dec a ; 4
            ex af, af' ; 4
                sub a, d ; 4
                jr nc, textureCont ; 8/9
                    moveTexturePointer:
                    inc hl ; 4
                    add a, e ; 4
                    jr nc, moveTexturePointer ; 8/9
                    ld b, (hl) ; 6 - 205 cycles (depending on flash) (most likely: 6, 7, or 14 cycles)
                textureCont:
            ex af, af' ; 4
        exx ; 4
        ; If the texel is 255 (the transparency color), skip drawing the pixel
        jr c, fill_cont ; 8/9
            lea bc, ix ; 12
            ld hl, 76800 ; 16
            add hl, bc ; 4
            ; Save the pixel value to c
            ld c, a ; 4
            selfModifyingCode1:
            ld a, 0 ; 8
            cp a, (hl) ; 8
            jr nc, left_fill_cont ; 8/9
                ld (hl), a ; 6
                ld (ix), c ; 14
            left_fill_cont:
            ld a, c ; 4
            scf ; 4
        fill_cont:
        ; Load x0 into BC
        ld bc, (iy + x0) ; 24
        ; Compare e2 to dy
        ld hl, (iy + dy) ; 24
        sbc hl, de ; 8
        ; If dy > e2, move on
        jp p, dy_cont ; 16/17
            ; Restore dy
            inc hl ; 4
            add hl, de ; 4
            ; Add dy to e2
            add hl, hl ; 4
            add hl, de ; 4
            ; Add sx to x0
            ld de, (iy + sx) ; 24
            add ix, de ; 8
            ; Put e2 into DE and sx into HL
            ex de, hl ; 4
            add hl, bc ; 4
            ld (iy + x0), hl ; 18
            ; Check if (the previous) x0 == x1
            ld hl, (iy + x1) ; 24
            or a, a ; 4
            sbc hl, bc ; 8
            ; If x0 == x1, jump out of the loop
            jr z, real_end ; 8/9
            ; If the texel is 255 (the transparency color), skip drawing the pixel
            cp a, 255 ; 8
            jr nc, fill_cont_2 ; 8/9
                lea bc, ix ; 12
                ld hl, 76800 ; 16
                add hl, bc ; 4
                ; Save the pixel value to c
                ld c, a ; 4
                selfModifyingCode2:
                ld a, 0 ; 8
                cp a, (hl) ; 8
                jr nc, fill_cont_2 ; 8/9
                    ld (hl), a ; 6
                    ld (ix), c ; 14
                    or a, a ; 4
            fill_cont_2:
        dy_cont:
        ; Compare e2 to dx
        ld hl, (iy + dx) ; 24
        sbc hl, de ; 8
        ; If e2 > dx, move on (Effectively jump to the beginning of the loop)
        jp m, new_fillLoop ; 16/17
            ; Restore dx
            add hl, de ; 4
            ; Add dx to e2
            add hl, hl ; 4
            add hl, de ; 4
            ; Add sy to y0
            ld bc, (iy + y0) ; 24
            ld de, (iy + sy) ; 24
            add ix, de ; 8
            ; Put sy into HL and e2 into DE
            ex de, hl ; 4
            add hl, bc ; 4
            ld (iy + y0), hl ; 18
            ; Check if (the previous) y0 == y1
            ld hl, (iy + y1) ; 24
            or a, a ; 4
            sbc hl, bc ; 8
            ; If y0 != y1, jump to the beginning of the loop
            jr nz, new_fillLoop ; 8/9
    real_end:
    ld sp, iy
    pop ix
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
section .text
public _polygonZShift
; Shift HL right by 4 and return a uint8_t
_polygonZShift:
    pop de ; 16
    ex (sp), hl ; 22
    add hl, hl ; 4
    add hl, hl ; 4
    add hl, hl ; 4
    add hl, hl ; 4
    ld a, h ; 4
    ex de, hl ; 4
    jp (hl) ; 6
section .text
public _polygonPointMultiply
; Takes a number and multiplies it by 7
_polygonPointMultiply:
    ld hl, 3 ; 16
    add hl, sp ; 4
    ld de, (hl) ; 20
    sbc hl, hl ; 8
    add hl, de ; 4
    ; thx calc84maniac on discord!
    add hl, hl ; 4
    add hl, de ; 4
    add hl, hl ; 4
    add hl, de ; 4
    ret ; 18
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