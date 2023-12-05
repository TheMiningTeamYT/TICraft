section .text
public _drawTextureLineNewA
; int startingX, int endingX, int startingY, int endingY, const uint8_t* texture, uint8_t colorOffset
_drawTextureLineNewA:
    di
    push ix
    ; Copy the arguments from the stack to the variable space
    ld iy, vars
    ld hl, 6
    add hl, sp
    lea de, iy
    ld bc, 19
    ldir

    ; BC will be sx
    inc bc
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
    ; Shift hl left 12
    add hl, hl
    add hl, hl
    add hl, hl
    add hl, hl
    add hl, hl
    add hl, hl
    add hl, hl
    add hl, hl
    add hl, hl
    add hl, hl
    add hl, hl
    add hl, hl
    ; Take the reciprocal of hl
    push hl
    ; 1
    ld hl, $001000
    push hl
    ; But first, some other stuff
    ; offset x0 & x1 by gfx_vram
    ld hl, (iy + x0)
    ld de, $D40000
    add hl, de
    ld (iy + x0), hl
    ld hl, (iy + x1)
    add hl, de
    ld (iy + x1), hl
    ; Pre-multiply y0 & y1
    ld bc, 320
    ld hl, (iy + y0)
    call __imuls_fast
    ld (iy + y0), hl
    ex de, hl
    add ix, de
    ld bc, 320
    ld hl, (iy + y1)
    call __imuls_fast
    ld (iy + y1), hl
    ; Init column & texture ratio & draw first pixel
    exx
        ; Divide
        call _fp_div
        ; Restore stack pointer
        inc sp
        inc sp
        inc sp
        inc sp
        inc sp
        inc sp
        ex de, hl
        dec de
        ld b, e
        ld e, d
        ld c, (iy + colorOffset)
        inc c
        ld hl, (iy + texture)
        ex af, af'
            xor a, a
            ld d, a
        ex af, af'
    exx
    ; At this point, all our registers/variables should be initialized
    ; hl': texture
    ; c': colorOffset
    ; b': (textureRatio << 12) & 0xF00000 (textureRatio % 1) (Fractional component of textureRatio)
    ; de': floor(textureRatio)
    ; a': Running total of the fractional component of column
    while_offscreen:
    ; init registers
    ld de, (iy + y0) ; 5
    ld bc, (iy + x0) ; 6

    ; check to make sure that x isn't off the screen to the right
    ; if it is, move x/y and try again
    ld hl, $D4013F ; 4
    or a, a ; 1
    sbc hl, bc ; 2
    jr c, update_x_y ; 2/3

    ; check to make sure that x isn't off the screen to the left
    ; if it is, move x/y and try again
    ld hl, $D3FFFF ; 4
    sbc hl, bc ; 2
    jr nc, update_x_y ; 2/3

    ; check to make sure that y isn't off the screen to the bottom or top
    ; if it is, move x/y and try again
    ld hl, 76481 ; 4
    sbc hl, de ; 2
    jr c, update_x_y ; 2/3

    ex af, af' ; 1
        ld ix, (iy + x0) ; 6
        add ix, de ; 2
    ex af, af' ; 1

    jp z, line_ends_off_screen ; 4/5

    add hl, de ; 1
    dec hl ; 1
    ld de, (iy + y1) ; 5
    sbc hl, de ; 2
    jr c, line_ends_off_screen ; 2/3

    ld de, (iy + x1) ; 5
    ld hl, $D3FFFF ; 4
    sbc hl, de ; 2
    jr nc, line_ends_off_screen ; 2/3

    ld hl, $D40140 ; 4
    sbc hl, de ; 2
    jr c, line_ends_off_screen ; 2/3
    
    jp line_ends_on_screen ; 5

    update_x_y:
    exx ; 1
        ex af, af' ; 1
            add a, b ; 1
            adc hl, de ; 2
        ex af, af' ; 1
    exx ; 1
    ; Test if x0 == x1
    ld hl, (iy + x1) ; 5
    or a, a ; 1
    sbc hl, bc ; 2
    ; If x0 != x1, move on
    jr nz, end_cont ; 2/3
        ; Else, test if y0 == y1
        ld hl, (iy + y1) ; 5
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
        ld hl, (iy + sx) ; 5
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
        ld hl, (iy + sy) ; 5
        add hl, bc ; 1
        ld (iy + y0), hl ; 6
    dx_cont:
    ; Jump to the beginning of the loop
    jp while_offscreen ; 5
    real_end:
    inc sp
    inc sp
    inc sp
    pop ix
    ei
    ret
line_ends_off_screen:
    ; Load the texel value and advance column
        exx ; 1
            ; Add the color offset to the pixel
            ld a, (hl) ; 2
            add a, c ; 1
            dec a ; 1
            ex af, af' ; 1
                add a, b ; 1
                adc hl, de ; 2
            ex af, af' ; 1
        exx ; 1
        ld bc, (iy + x0) ; 6
        ; If the texel is 255 (the transparency color), skip drawing the pixel
        jr c, fill_cont_off ; 2/3
            ; check to make sure that y isn't off the screen to the top or bottom
            ; if it is, don't draw the pixel
            ; also used to make sure we don't draw off the screen
            ld de, (iy + y0) ; 5
            ld hl, 76480 ; 4
            sbc hl, de ; 2
            jr c, real_end ; 2/3
                ex af, af' ; 1
                    ld hl, 76800 ; 4
                    add hl, de ; 1
                    add hl, bc ; 1
                    ld d, a ; 1
                    ld a, (iy + polygonZ) ; 4
                    cp a, (hl) ; 2
                    jr nc, fill_upper_cont_off ; 2/3
                        ld (hl), a ; 2
                        ex af, af' ; 1
                        ld (ix), a ; 4
                        ex af, af' ; 1
                    fill_upper_cont_off:
                    ld a, d ; 1
                ex af, af' ; 1
                jr z, fill_cont_off ; 2/3
                ld de, 320 ; 4
                add hl, de ; 1
                ld e, a ; 1
                ld a, (iy + polygonZ) ; 4
                cp a, (hl) ; 2
                jr nc, fill_lower_cont_off ; 2/3
                    ld (hl), a ; 2
                    ld a, e ; 1
                    ld e, d ; 1
                    lea hl, ix + 63 ; 3
                    add hl, de ; 1
                    ld (hl), a ; 2
                fill_lower_cont_off:
        fill_cont_off:
        ; Test if x0 == x1
        ld hl, (iy + x1) ; 5
        or a, a ; 1
        sbc hl, bc ; 2
        ; If x0 != x1, move on
        jr nz, end_cont_off ; 2/3
            ; Else, test if y0 == y1
            ld hl, (iy + y0) ; 5
            ld de, (iy + y1) ; 5
            sbc hl, de ; 2
            ; If y0 == y1 as well, jump out of the loop
            jr z, real_end ; 2/3
        end_cont_off:
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
        jp m, dy_cont_off ; 4/5
            ; Check if x0 == x1
            ld hl, (iy + x1) ; 5
            or a, a ; 1
            sbc hl, bc ; 2
            ; If x0 == x1, jump out of the loop
            jr z, real_end_off ; 2/3
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
            ex de, hl ; 1

            ; check to make sure that x isn't off the screen to the right
            ; if it is, jump out of the loop
            ld hl, $D4013F ; 4
            or a, a ; 1
            sbc hl, de ; 2
            jr c, real_end_off ; 2/3

            ; check to make sure that x isn't off the screen to the left
            ; if it is, jump out of the loop
            ld hl, $D3FFFF ; 4
            sbc hl, de ; 2
            jr nc, real_end_off ; 2/3
            
            ld (iy + x0), de ; 6
        dy_cont_off:
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
        jp p, dx_cont_off ; 4/5
            ; Check if y0 == y1
            ld hl, (iy + y1) ; 5
            ld bc, (iy + y0) ; 6
            or a, a ; 1
            sbc hl, bc ; 2
            ; If y0 == y1, jump out of the loop
            jr z, real_end_off ; 2/3
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
        dx_cont_off:
        ; Jump to the beginning of the loop
        jp line_ends_off_screen ; 5
    real_end_off:
    jp real_end ; 5
line_ends_on_screen:
    ; Load the texel value and advance column
    exx ; 1
        ; Add the color offset to the pixel
        ld a, (hl) ; 2
        add a, c ; 1
        ex af, af' ; 1
            add a, b ; 1
            adc hl, de ; 2
        ex af, af' ; 1
    exx ; 1
    ; If the texel is 255 (the transparency color), skip drawing the pixel
    jr c, fill_cont_on ; 2/3
        dec a ; 1
        lea hl, ix ; 3
        ld de, 76800 ; 4
        add hl, de ; 1
        ld c, a ; 1
        ld a, (iy + polygonZ) ; 4
        cp a, (hl) ; 2
        jr nc, fill_upper_cont_on ; 2/3
            ld (hl), a ; 2
            ld (ix), c ; 4
        fill_upper_cont_on:
        ld de, 320 ; 4
        add hl, de ; 1
        cp a, (hl) ; 2
        jr nc, fill_lower_cont_on ; 2/3
            ld (hl), a ; 2
            lea hl, ix ; 3
            add hl, de ; 1
            ld (hl), c ; 4
        fill_lower_cont_on:
    fill_cont_on:
    ; Test if x0 == x1
    ld hl, (iy + x1) ; 5
    ld bc, (iy + x0) ; 6
    or a, a ; 1
    sbc hl, bc ; 2
    ; If x0 != x1, move on
    jr nz, end_cont_on ; 2/3
        ; Else, test if y0 == y1
        ld hl, (iy + y0) ; 5
        ld de, (iy + y1) ; 5
        sbc hl, de ; 2
        ; If y0 == y1 as well, jump out of the loop
        jr z, real_end_on ; 2/3
    end_cont_on:
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
    jp m, dy_cont_on ; 4/5
        ; Check if x0 == x1
        ld hl, (iy + x1) ; 5
        or a, a ; 1
        sbc hl, bc ; 2
        ; If x0 == x1, jump out of the loop
        jr z, real_end_on ; 2/3
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
    dy_cont_on:
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
    jp p, dx_cont_on ; 4/5
        ; Check if y0 == y1
        ld hl, (iy + y1) ; 5
        ld bc, (iy + y0) ; 6
        or a, a ; 1
        sbc hl, bc ; 2
        ; If y0 == y1, jump out of the loop
        jr z, real_end_on ; 2/3
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
    dx_cont_on:
    ; Jump to the beginning of the loop
    jp line_ends_on_screen ; 5
    real_end_on:
    jp real_end ; 5
section .data
private gfx_vram
gfx_vram = $D40000
extern vars
extern abs
extern _fp_div
extern _fp_mul
extern __idvrmu
extern __imuls_fast
private x0
x0 = 0
private x1
x1 = 3
private y0
y0 = 6
private y1
y1 = 9
private texture
texture = 12
private colorOffset
colorOffset = 15
private polygonZ
polygonZ = 18
private dx
dx = 19
private sx
sx = 23
private dy
dy = 26
private sy
sy = 29
private error
error = 32