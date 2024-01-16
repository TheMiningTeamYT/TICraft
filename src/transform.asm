section .text
assume adl = 1
section .text
; screenPoint transformPoint(point x)
public _transformPointNewA
_transformPointNewA:
    ; this function assumes sz is 0 and cz is 1
    ; but also, not sure why you'd want to rotate the camera like that
    di
    push ix
    ld iy, 9
    add iy, sp
    exx
        lea hl, iy-9
        ld de, (_sx)
        ld bc, 3
    exx
    ; Compute (distance from camera on) x, y, and z
    ; and compute the values that will be needed for the math later

    ; x, czx, and szx
    ld hl, (iy + x)
    ld de, (_cameraXYZ + x)
    or a, a
    sbc hl, de
    ld (iy + czx), hl
    push hl
    call _fp_to_int
    ld (iy + x), hl

    ; y, czy, and szy
    ld hl, (iy + y)
    ld de, (_cameraXYZ + y)
    or a, a
    sbc hl, de
    ld (iy + czy), hl
    push hl
    call _fp_to_int
    ld (iy + y), hl

    exx
        ld sp, hl
        or a, a
        sbc hl, bc
    exx

    ld hl, (iy + zz)
    ld de, (_cameraXYZ + zz)
    or a, a
    sbc hl, de
    push hl
    call _fp_to_int
    ld (iy + zz), hl
    ; somewhere in here could you put a check for z > zCulllingDistance?
    ld hl, (_cy)
    push hl
    call _fp_mul
    exx
        ; restore stack pointer
        ld sp, hl
        add hl, bc
    exx
    ld (iy + cyz), hl
    ld hl, (_sy)
    push hl
    call _fp_mul
    exx
        ; restore stack pointer
        ld sp, hl
        or a, a
        sbc hl, bc
    exx
    ld ix, (iy - 3)

    ; calculate dx
    ; save syz to the stack
    push hl
    ; czX (sum1)
    ld hl, (iy + czx)
    push hl
    ; cy*sum1
    ld hl, (_cy)
    push hl
    call _fp_mul
    exx
        ; restore stack pointer
        ld sp, hl
        add hl, bc
    exx
    pop de
    sbc hl, de
    ld (iy + dx), hl

    ; calulate dy
    ; sy*sum1
    ld hl, (iy + czx)
    push hl
    ld hl, (_sy)
    push hl
    call _fp_mul
    exx
        ; restore stack pointer
        ld sp, hl
        push de
    exx
    ; cyz + sy*sum1 (sum2)
    ld de, (iy + cyz)
    add hl, de
    ld (iy + sum2), hl
    push hl
    call _fp_mul
    exx
        ; restore stack pointer
        ld sp, hl
        or a, a
        sbc hl, bc
    exx
    ; save sx*(sum2) to the stack
    push hl
    ; czy (sum3)
    ld hl, (iy + czy)
    ld de, (_cx)
    push de
    push hl
    call _fp_mul
    exx
        ; restore stack pointer
        ld sp, hl
        or a, a
        sbc hl, bc
    exx
    pop de
    add hl, de
    ld (iy + dy), hl

    ; calculate dz
    ; cx*sum2
    exx
        ; restore stack pointer
        ld sp, hl
        add hl, bc
        add hl, bc
    exx
    ld hl, (iy + sum2)
    push hl
    call _fp_mul
    exx
        ; restore stack pointer
        ld sp, hl
        or a, a
        sbc hl, bc
    exx
    ; save cx*sum2 to the stack
    push hl
    ; sx*sum3
    ld hl, (iy + czy)
    push hl
    exx
        push de
    exx
    call _fp_mul
    exx
        ld sp, hl
        add hl, bc
    exx
    ex de, hl
    pop hl
    sbc hl, de ; dz

    ; Compute the final x/y values
    push hl
    ld hl, focalLength
    push hl
    call _fp_div
    exx
        ld sp, hl
        or a, a
        sbc hl, bc
    exx
    push hl
    ; write screenPoint.x
    ld hl, (iy + dx)
    push hl
    call _fp_mul
    push hl
    call _fp_to_int
    exx
        ld sp, hl
        add hl, bc
    exx
    ld de, 160
    add hl, de
    ld (ix), hl
    ; write screenPoint.y
    ld hl, (iy + dy)
    push hl
    call _fp_mul
    push hl
    call _fp_to_int
    exx
        ld sp, hl
    exx
    ex de, hl
    ld hl, 120
    or a, a
    sbc hl, de
    ld (ix + 2), hl

    ; finally, compute the Z
    ; use the stack to keep a running total
    ; x*x
    ld hl, (iy + x)
    ld bc, (iy + x)
    call __imuls_fast
    push hl
    ; y*y
    ld hl, (iy + y)
    ld bc, (iy + y)
    call __imuls_fast
    pop de
    add hl, de
    push hl
    ; z*z
    ld hl, (iy + zz)
    ld bc, (iy + zz)
    call __imuls_fast
    pop de
    add hl, de
    
    ;push hl
    ;ld hl, 2
    ;add hl, sp
    ;ld hl, sp
    ;inc sp
    ;ld d, (hl)
    ;dec hl
    ;ld a, (hl)
    ;srl d
    ;rra
    ;srl d
    ;rra
    ;srl d
    ;rra
    ; write the z value
    ;ld (ix + 4), a
    ;ld (ix + 5), d
    call approx_sqrt_a
    ld (ix + 5), h
    ld (ix + 4), l

    ; put the pointer to the result in hl
    lea hl, ix
    ; restore the index registers
    pop ix
    ret
section .data
extern _fp_div
extern _fp_mul
extern _cameraXYZ
extern _fp_to_int
extern __imuls_fast
extern approx_sqrt_a
extern _cx
extern _sx
extern _cy
extern _sy
private focalLength
focalLength = $AB60B
private x
x = 0
private y
y = 3
private zz 
zz  = 6
private sum1
sum1 = -39
private sum2
sum2 = -42
private sum3
sum3 = -45
private dx
dx = -48
private dy
dy = -51
private dz
dz = -54
private czx
czx = -57
private szx
szx = -60
private czy
czy = -63
private szy
szy = -66
private cyz
cyz = -69
private syz
syz = -72