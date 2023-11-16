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
public _drawTextureLineNewA
; int startingX, int endingX, int startingY, int endingY, const uint8_t* texture, uint8_t colorOffset
_drawTextureLineNewA:
    di
    ; Save the index registers
    push ix
    ; Copy the arguments from the stack to the variable space
    ld iy, 0
    ld hl, 6
    add hl, sp
    ld de, vars
    add iy, de
    ld bc, 18
    ldir
    ; BC will be sx
    ld bc, 1
    ; Compute dx
    ld hl, (iy + x1)
    dec hl
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
    inc hl
    ; Compute abs(dx)
    call abs
    ; Store dx
    ld (iy + dx), hl
    ; BC will be sy
    ld bc, 320
    ; Compute dy
    ld hl, (iy + y1)
    dec hl
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
    inc hl
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
    ; Store dx + dy to error
    ld (iy + error), hl
    ; Retrieve abs(dy) from the stack
    pop hl
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
    ; Divide ~15.99 by hl
    push hl
    ; ~15.99
    ld hl, $00FFFF
    push hl
    ; But first, some other stuff
    ; Pre-multiply y0 & y1
    ld de, 160
    ld h, (iy + y0)
    ld l, e
    mlt hl
    add hl, hl
    ld (iy + y0), hl
    ex de, hl
    ld h, (iy + y1)
    mlt hl
    add hl, hl
    ld (iy + y1), hl
    ; offset x by gfx_vram
    ld de, gfx_vram
    ld hl, (iy + x0)
    add hl, de
    ld (iy + x0), hl
    ld hl, (iy + x1)
    add hl, de
    ld (iy + x1), hl
    ; Init column & texture ratio & draw first pixel
    exx
        ; Divide
        call _fp_div
        dec hl
        ; load whole number part of texture ratio into c
        ld bc, 0
        ld c, h ; middle 8 bits of texture ratio
        srl c
        srl c
        srl c
        srl c
        ; c now contains the whole number portion of texture ratio
        ; shift texture ratio left 12
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
        ; put texture ratio into de
        ex de, hl
        ; Restore the stack pointer
        inc sp
        inc sp
        inc sp
        inc sp
        inc sp
        inc sp
        ; Init hl to 0
        or a, a
        sbc hl, hl
        ; Put colorOffset into l
        ld l, (iy + colorOffset)
        dec l
        ; Init IX
        ld ix, (iy + texture)
    exx
    ; At this point, all our variables should be initialized
    ; hl': column << 12 (0)
    ; l': colorOffset
    ; de': textureRatio << 12
    ; bc': floor(textureRatio)
    ; ix: texture
    ; 78-191 cycles per pixel
    ; a LOT of memory access going on here that could potentially be optimized
    new_fillLoop:
        ; Load the pixel value
        ld a, (ix) ; 4
        inc a ; 1
        ; If it is 255 (the transparency color), skip drawing the pixel
        jr z, fill_cont ; 2/3
            ; Add the color offset to the pixel
            exx ; 1
            add a, l ; 1
            exx ; 1
            ; init hl to y0
            ld hl, (iy + y0) ; 5
            ; We're going to use hl as a pointer to the location on the screen to write to
            ; Add the x value to hl
            ld de, (iy + x0) ; 5
            add hl, de ; 1
            ; Write the pixel
            ld (hl), a ; 2
            ; Move down 1 row
            ld de, 320 ; 4
            add hl, de ; 1
            ; Write the pixel (again)
            ld (hl), a ; 2
        fill_cont:
        ; Advance column
        exx ; 1
            add ix, bc ; 2
            add hl, de ; 1
            jr nc, ix_cont ; 2/3
                inc ix ; 1
            ix_cont:
        exx ; 1
        ; Test if x0 == x1
        ld hl, (iy + x0) ; 5
        ld de, (iy + x1) ; 5
        or a, a ; 1
        sbc hl, de ; 2
        ; If x0 != x1, move on
        jr nz, end_cont ; 2/3
            ; Else, test if y0 == y1
            ld hl, (iy + y0) ; 5
            ld de, (iy + y1) ; 5
            sbc hl, de ; 2
            ; If y0 == y1 as well, jump out of the loop
            jr z, real_end ; 2/3
        end_cont:
        ; Multiply error by 2 (e2)
        ld hl, (iy + error) ; 5
        add hl, hl ; 1
        ; At this point, e2 is in hl
        ; Save e2 to the stack
        push hl ; 4
        ; Compare e2 to dy
        ld de, (iy + dy) ; 5
        or a, a ; 1
        sbc hl, de ; 2
        ; If dy > e2, move on
        jp m, dy_cont ; 4/5
            ; Check if x0 == x1
            ld hl, (iy + x1) ; 5
            ld bc, (iy + x0) ; 6
            or a, a ; 1
            sbc hl, bc ; 2
            ; If x0 == x1, jump out of the loop
            jr z, real_end ; 2/3
            ; Else, add sx to x0
            ld hl, (iy + sx) ; 5
            add hl, bc ; 1
            ld (iy + x0), hl ; 6
            ; Add dy to error
            ld hl, (iy + error) ; 5
            add hl, de ; 1
            ld (iy + error), hl ; 6
        dy_cont:
        ; Retrieve e2 from the stack
        pop hl ; 4
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
            ; Else, add sy to y0
            ld hl, (iy + sy) ; 5
            add hl, bc ; 1
            ld (iy + y0), hl ; 6
            ; Add dx to error
            ld hl, (iy + error) ; 5
            add hl, de ; 1
            ld (iy + error), hl ; 6
        dx_cont:
        ; Jump to the beginning of the loop
        jp new_fillLoop ; 5
    real_end:
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
section .data
private gfx_vram
gfx_vram = $D40000
public vars
vars: db 45 dup 00h
private startingSP
startingSP: db 3 dup 00h
extern _fp_div
extern _fp_mul
extern __idvrmu
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
private dx
dx = 18
private sx
sx = 21
private dy
dy = 24
private sy
sy = 27
private error
error = 30