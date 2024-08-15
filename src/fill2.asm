assume adl=1
; The majority of the cycle counts here are wrong
; because I wrote them before I realized how long instructions truely take
; on the TI 84 Plus CE.
; This is something that needs to be fixed
; I just removed the check on each pixel fill loop to see if we're at the end of the line
; in favor of relying entirely on the checks we do when we advance x or y
; It seems to be fine so far but it could result in problems down the line
section .text
; An implementation of Bresenham's line algorithm based on the psuedo-code from Wikipedia
; https://en.wikipedia.org/wiki/Bresenham%27s_line_algorithm
public _drawTextureLineNewA
; int startingX, int endingX, int startingY, int endingY, const uint8_t* texture, uint8_t colorOffset
_drawTextureLineNewA:
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
    jp p, sx_cont
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
    jp p, sy_cont
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
    ; Push error*2 to the stack
    push hl
    ; Retrieve abs(dy) from DE
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
    ; Pre-multiply y0 & y1
    ld de, (iy + y0)
    ; 44 cycles to multiply any number in DE by 320
    ; not bad
    ld h, d
    ld l, e
    add hl, hl
    add hl, hl
    add hl, de
    add hl, hl
    add hl, hl
    add hl, hl
    add hl, hl
    add hl, hl
    add hl, hl
    ld (iy + y0), hl
    ld de, (iy + y1)
    ld h, d
    ld l, e
    add hl, hl
    add hl, hl
    add hl, de
    add hl, hl
    add hl, hl
    add hl, hl
    add hl, hl
    add hl, hl
    add hl, hl
    ld (iy + y1), hl
    ; Modify the self-modifying code
    ld a, (iy + polygonZ)
    ld (selfModifyingCode_off + 1), a
    ld (selfModifyingCode_off_2 + 1), a
    ld (selfModifyingCode_on + 1), a
    ld (selfModifyingCode_on_2 + 1), a
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
    exx
    jr while_offscreen
    ; At this point, all our registers/variables should be initialized
    ; a': texture error
    ; b': texel value
    ; c': color offset
    ; d': shifted texture length
    ; e': shifted length of the textured line being drawn
    ; hl': texture pointer
    dx_cont:
    ; Push e2 back onto the stack
    push de ; 10
    while_offscreen:
    ; init BC
    ld bc, (iy + x0) ; 24

    ; check to make sure that x isn't off the screen to the left or right
    ; if it is, move x/y and try again
    ld hl, 319 ; 16
    or a, a ; 4
    sbc hl, bc ; 8
    jr c, update_x_y ; 8/9

    ; init DE
    ld de, (iy + y0) ; 24

    ; check to make sure that y isn't off the screen to the bottom or top
    ; if it is, move x/y and try again
    ld hl, 76480 ; 16
    sbc hl, de ; 8
    jr c, update_x_y ; 8/9

    exx ; 4
        ld b, (hl) ; 6 - 205 cycles (depending on flash) (most likely: 6, 7, or 14 cycles)
    exx ; 4
    ld ix, gfx_vram ; 20
    add ix, bc ; 8
    add ix, de ; 8

    ; If the ending y position is off the screen, jump to the code for if the line ends off screen
    ; Restore hl to 76480
    ; Worried this might require clearing the carry flag, but I don't think it will/
    add hl, de ; 4
    ld de, (iy + y1) ; 24
    sbc hl, de ; 8
    jr c, line_ends_off_screen ; 8/9

    ; If the ending x position is off the screen, jump to the code for if the line ends off screen
    ; Set hl to 319
    ld hl, 319 ; 16
    ld bc, (iy + x1) ; 24
    sbc hl, bc ; 8/9
    jr c, line_ends_off_screen ; 8/9
    
    jp line_ends_on_screen ; 17
update_x_y:
    ; Advance the texture pointer
    exx ; 4
        ex af, af' ; 4
            sub a, d ; 4
            jr nc, textureCont ; 8/9
                moveTexturePointer:
                inc hl ; 4
                add a, e ; 4
                jr nc, moveTexturePointer ; 8/9
            textureCont:
        ex af, af' ; 4
    exx ; 4
    ; Grab e2 from the stack
    pop de ; 16
    ; Compare e2 to dy
    ld hl, (iy + dy) ; 24
    scf ; 4
    sbc hl, de ; 8
    ; If dy > e2, move on
    jp p, dy_cont ; 16/17
        ; Restore dy
        inc hl ; 4
        add hl, de ; 4
        ; Add dy to e2
        add hl, hl ; 4
        add hl, de ; 4
        ; Save e2 to DE
        ex de, hl ; 4
        ; Check if x0 == x1
        ld hl, (iy + x1) ; 24
        or a, a ; 4
        sbc hl, bc ; 8
        ; If x0 == x1, jump out of the loop
        jr z, real_end ; 8/9
        ; Add sx to x0
        ld hl, (iy + sx) ; 24
        add hl, bc ; 4
        ld (iy + x0), hl ; 18
        or a, a ; 4
    dy_cont:
    ; Compare e2 to dx
    ld hl, (iy + dx) ; 24
    sbc hl, de ; 8
    ; If e2 > dx, move on (Effectively jump to the beginning of the loop)
    jp m, dx_cont ; 16/17
        ; Restore dx
        add hl, de ; 4
        ; Add dx to e2
        add hl, hl ; 4
        add hl, de ; 4
        ; Save e2 to the stack
        push hl ; 10
        ; Add sy to y0
        ld bc, (iy + y0) ; 24
        ld hl, (iy + sy) ; 24
        add hl, bc ; 4
        ld (iy + y0), hl ; 18
        ; Check if y0 == y1
        ld hl, (iy + y1) ; 24
        or a, a ; 4
        sbc hl, bc ; 8
        ; If y0 != y1, jump to the beginning of the loop
        jp nz, while_offscreen ; 16/17
    real_end:
    ld sp, iy ; 8
    pop ix ; 20
    ret ; 18
line_ends_off_screen:
    ; Pop e2 from the stack
    pop de ; 10
new_fill_loop_off:
    ; Load the texel value and advance the texture pointer
    exx ; 4
        ; Load the texel value
        ld a, b ; 4
        ; Add the color offset to the pixel
        add a, c ; 4
        dec a ; 4
        ex af, af' ; 4
            sub a, d ; 4
            jr nc, textureCont_off ; 8/9
                moveTexturePointer_off:
                inc hl ; 4
                add a, e ; 4
                jr nc, moveTexturePointer_off ; 8/9
                ld b, (hl) ; 6 - 205 cycles (depending on flash) (most likely: 6, 7, or 14 cycles)
            textureCont_off:
        ex af, af' ; 4
    exx ; 4
    ; If the texel is 255 (the transparency color), skip drawing the pixel
    jr c, fill_cont_off ; 8/9
        ; also used to make sure we don't draw off the screen
        ; Decrementing A now to make sure we don't overwrite the Z flag later
        lea bc, ix ; 12
        ld hl, 76800 ; 16
        add hl, bc ; 4
        ld c, a ; 4
        selfModifyingCode_off:
        ld a, 0 ; 8
        cp a, (hl) ; 8
        jr nc, left_fill_cont_off ; 8/9
            ld (hl), a ; 6
            ld (ix), c ; 6
        left_fill_cont_off:
        ld a, c ; 4
        scf ; 4
    fill_cont_off:
    ; Load x0 into BC
    ld bc, (iy + x0) ; 24
    ; Compare e2 to dy
    ld hl, (iy + dy) ; 24
    sbc hl, de ; 8
    jp p, dy_cont_off ; 16/17
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
        ; check to make sure that x isn't off the screen to the left or right
        ; if it is, jump out of the loop
        ld bc, 320 ; 16
        sbc hl, bc ; 8
        jr nc, real_end ; 8/9
        ld hl, 76799 ; 16
        cp a, l ; 4
        jr nc, fill_cont_off_2;  8/9
            lea bc, ix + 1; 12
            add hl, bc ; 4
            ld c, a ; 4
            selfModifyingCode_off_2:
            ld a, 0 ; 8
            cp a, (hl) ; 8
            jr nc, fill_cont_off_2 ; 8/9
                ld (hl), a ; 6
                ld (ix), c ; 14
                or a, a ; 4
        fill_cont_off_2:
    dy_cont_off:
    ; Compare e2 to dx
    ld hl, (iy + dx) ; 24
    sbc hl, de ; 8
    ; If e2 > dx, move on (Effectively jump to the beginning of the loop)
    jp m, new_fill_loop_off ; 16/17
        ; Restore dx
        add hl, de ; 4
        ; Add dx to e2
        add hl, hl ; 4
        add hl, de ; 4
        ; Save e2 to de
        ex de, hl ; 4
        ; Add sy to y0
        ld hl, (iy + y0) ; 24
        ld bc, (iy + sy) ; 24
        add ix, bc ; 8
        add hl, bc ; 4
        ; check to make sure that y isn't off the screen to the top or bottom
        ; if it is, break out of the loop
        ld (iy + y0), hl ; 18
        ld bc, 76481 ; 16
        sbc hl, bc ; 8
        jr c, new_fill_loop_off ; 16/17
    real_end_off:
    ld sp, iy ; 8
    pop ix ; 20
    ret ; 18
line_ends_on_screen:
    ; Pop e2 from the stack
    pop de ; 16
new_fill_loop_on:
    ; Load the texel value and advance the texture pointer
    exx ; 4
        ; Load the texel value
        ld a, b ; 4
        dec a ; 4
        ; Add the color offset to the pixel
        add a, c ; 4
        ex af, af' ; 4
            sub a, d ; 4
            jr nc, textureCont_on ; 8/9
                moveTexturePointer_on:
                inc hl ; 4
                add a, e ; 4
                jr nc, moveTexturePointer_on ; 8/9
                ld b, (hl) ; 6 - 205 cycles (depending on flash) (most likely: 6, 7, or 14 cycles)
            textureCont_on:
        ex af, af' ; 4
    exx ; 4
    ; If the texel is 255 (the transparency color), skip drawing the pixel
    jr c, fill_cont_on ; 8/9
        lea bc, ix ; 12
        ld hl, 76800 ; 16
        add hl, bc ; 4
        ; Save the pixel value to c
        ld c, a ; 4
        selfModifyingCode_on:
        ld a, 0 ; 8
        cp a, (hl) ; 8
        jr nc, left_fill_cont_on ; 8/9
            ld (hl), a ; 6
            ld (ix), c ; 6
        left_fill_cont_on:
        ld a, c ; 4
        scf ; 4
    fill_cont_on:
    ; Load x0 into BC
    ld bc, (iy + x0) ; 24
    ; Compare e2 to dy
    ld hl, (iy + dy) ; 24
    sbc hl, de ; 8
    ; If dy > e2, move on
    jp p, dy_cont_on ; 16/17
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
        jr z, real_end_on ; 8/9
        ; If the texel is 255 (the transparency color), skip drawing the pixel
        cp a, 255 ; 8
        jr nc, fill_cont_2_on ; 8/9
            lea bc, ix ; 12
            ld hl, 76800 ; 16
            add hl, bc ; 4
            ; Save the pixel value to c
            ld c, a ; 4
            selfModifyingCode_on_2:
            ld a, 0 ; 8
            cp a, (hl) ; 8
            jr nc, fill_cont_2_on ; 8/9
                ld (hl), a ; 6
                ld (ix), c ; 14
                or a, a ; 4
        fill_cont_2_on:
    dy_cont_on:
    ; Compare e2 to dx
    ld hl, (iy + dx) ; 24
    sbc hl, de ; 8
    ; If e2 > dx, move on (Effectively jump to the beginning of the loop)
    jp m, new_fill_loop_on ; 16/17
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
        jr nz, new_fill_loop_on ; 8/9
    real_end_on:
    ld sp, iy ; 8
    pop ix ; 20
    ret ; 18
section .data
private gfx_vram
gfx_vram = $D40000
extern abs
extern _fp_div
extern _fp_mul
extern __idvrmu
extern __imuls_fast
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