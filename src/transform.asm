section .text
assume adl = 1
public ___ZN6object14generatePointsEv
___ZN6object14generatePointsEv:
    ; save ix to the stack
    push ix
    ; set iy to SP
    ld iy, 0
    add iy, sp
    ; move the stack pointer up
    lea hl, iy - 90
    ld sp, hl
    ; set ix to point to the cube
    ld ix, (iy + objectPointer)
    ; set visible to false
    ld (ix + visible), 0
    ; setup x1/x2/y1/y2/z1/z1
    ; (Fixed24)20
    ld bc, 81920
    ; (Fixed24)x - cameraXYZ[0]
    ld hl, (ix + objectX - 1)
    ; convert int to fixed24
    ld l, 0
    add hl, hl
    add hl, hl
    add hl, hl
    add hl, hl
    ld de, (_cameraXYZ)
    or a, a
    sbc hl, de
    ld (iy + x1), hl
    push hl
    ; x1 + 20
    add hl, bc
    ld (iy + x2), hl
    push hl
    ; (Fixed24)y - cameraXYZ[1]
    ld hl, (ix + objectY - 1)
    ld l, 0
    add hl, hl
    add hl, hl
    add hl, hl
    add hl, hl
    ld de, (_cameraXYZ + 3)
    or a, a
    sbc hl, de
    ld (iy + y1), hl
    push hl
    ; if y1 <= 0, continue
    jr z, y1_cont
    jp m, y1_cont
        exx
            ld hl, (_angleX)
            ld a, (_angleX + 3)
            ld e, a
            ld a, $42
            ld bc, $340000
            call __fcmp
        exx
        jp p, functionEnd
    y1_cont:
    ; y1 - 20
    or a, a
    sbc hl, bc
    ld (iy + y2), hl
    push hl
    ; (Fixed24)z - cameraXYZ[2];
    ld hl, (ix + objectZ - 1)
    ld l, 0
    add hl, hl
    add hl, hl
    add hl, hl
    add hl, hl
    ld de, (_cameraXYZ + 6)
    or a, a
    sbc hl, de
    ld (iy + z1), hl
    push hl
    ; z1 + 20
    add hl, bc
    ld (iy + z2), hl
    push hl
    ; What's on the stack right now:
    ; z2
    ; z1
    ; y2
    ; y1
    ; x2
    ; x1
    ; Convert z2 to an int
    call _fp_to_int
    ; Take z2 off the stack
    inc sp
    inc sp
    inc sp
    ; Take its square
    push hl
    pop bc
    call __imulu_fast
    ld (iy + z2squared), hl
    ; Convert z1 to an int
    call _fp_to_int
    ; Put z1 into bc'
    exx
        pop bc
    exx
    ; Take its square
    push hl
    pop bc
    call __imulu_fast
    ld (iy + z1squared), hl
    ; Convert y2 to an int
    call _fp_to_int
    ; Take y2 off the stack
    inc sp
    inc sp
    inc sp
    ; Take its square
    push hl
    pop bc
    call __imulu_fast
    ld (iy + y2squared), hl
    ; Convert y1 to an int
    call _fp_to_int
    ; Put y1 into de'
    exx
        pop de
    exx
    ; Take its square
    push hl
    pop bc
    call __imulu_fast
    ld (iy + y1squared), hl
    ; Convert x2 to an int
    call _fp_to_int
    ; Take x2 off the stack
    inc sp
    inc sp
    inc sp
    ; Take its square
    push hl
    pop bc
    call __imulu_fast
    ld (iy + x2squared), hl
    ; Convert x1 to an int
    call _fp_to_int
    ; Put x1 into hl'
    exx
        pop hl
    exx
    ; Take its square
    push hl
    pop bc
    call __imulu_fast
    ld (iy + x1squared), hl
    ; approx_sqrt_a(x1squared + y1squared + x1squared)
    ld de, (iy + y1squared)
    add hl, de
    ld de, (iy + z1squared)
    add hl, de
    call approx_sqrt_a
    ex.sis de, hl
    ld (ix + 4), de
    ; If renderedPoints[0].z > zCullingDistance, return
    ld hl, 2000
    or a, a
    sbc hl, de
    ; Could use carry instead of minus here but I don't think there's any point
    jp m, functionEnd
    ; Calculate the other constant coefficients
    ; cy*z1
    ld hl, (_cy)
    push hl
    exx
        ; bc' contains z1
        push bc
    exx
    call _fp_mul
    ld (iy + cyz1), hl
    ; Restore the stack pointer (leaving cy on the stack)
    lea hl, iy - 93
    ld sp, hl
    ; sy*x1
    ld hl, (_sy)
    push hl
    exx
        ; hl' contains x1
        push hl
    exx
    call _fp_mul
    ld (iy + syx1), hl
    ; Calculate sum1 (cyz1 + syx1)
    ld de, (iy + cyz1)
    add hl, de
    ; We're doing this because we're gonna need sum1 in a second
    ex de, hl
    ld (iy + sum1), de
    ; Restore the stack pointer (leaving cy & sy on the stack)
    lea hl, iy - 96
    ld sp, hl
    ; cx*sum1
    push de
    ld hl, (_cx)
    push hl
    call _fp_mul
    ; We're doing this because we're gonna need cx*sum1 in a second
    ex de, hl
    ; Restore the stack pointer
    lea hl, iy - 96
    ld sp, hl
    push de
    ; -sx*y1
    exx
        ; de' contains y1
        push de
    exx
    ld de, (_sx)
    or a, a
    sbc hl, hl
    sbc hl, de
    push hl
    call _fp_mul
    ; We're doing this because we're gonna need nsxy1 in a second
    ex de, hl
    ld (iy + nsxy1), de
    ; Restore the stack pointer
    lea hl, iy - 99
    ld sp, hl
    ; Grab cx*sum1 off the stack
    pop hl
    ; (cx*sum1) + nsxy1
    add hl, de
    ld (iy + dz), hl
    ; If dz is less than or equal to 20, return
    ex de, hl
    ld hl, 81920
    or a, a
    sbc hl, de
    jp p, functionEnd
    ; sy * z1, with sy still on the stack
    exx
        push bc
        ; bc' now contains z2
        ld bc, (iy + z2)
    exx
    call _fp_mul
    ; We're doing this because we're gonna need syz1 in a sec
    ex de, hl
    ld (iy + syz1), de
    ; Restore the stack pointer
    lea hl, iy - 93
    ld sp, hl
    ; Put syz1 on the stack and grab cy from the stack
    ex de, hl
    ex (sp), hl
    ; -cy*x1
    ex de, hl
    or a, a
    sbc hl, hl
    sbc hl, de
    push hl
    exx
        push hl
        ; hl' now contains x2
        ld hl, (iy + x2)
    exx
    call _fp_mul
    ; We're doing this because we're gonna need ncyx1 in a second
    ex de, hl
    ld (iy + ncyx1), de
    ; Restore the stack pointer (keeping -cy on the stack)
    lea hl, iy - 96
    ld sp, hl
    ; Grab -cy off the stack
    pop hl
    ; Grab syz1 off the stack and put -cy back on the stack
    ex (sp), hl
    ; ncyx1 + syz1
    add hl, de
    ld (iy + dx), hl
    ; Get the absolute value of dx
    call abs
    ; If dz + 20 < abs(dx), return
    ex de, hl
    ld hl, (iy + dz)
    ld bc, 81920
    add hl, bc
    ex de, hl
    or a, a
    sbc hl, de
    jp p, functionEnd
    ; -cy*x2
    exx
        ; hl' contains x2
        push hl
    exx
    call _fp_mul
    ld (iy + ncyx2), hl
    ; sy * x2
    ld hl, (_sy)
    ld (iy - 93), hl
    call _fp_mul
    ld (iy + syx2), hl
    ; sy * z2
    exx
        ; bc' contains z2
        ld (iy - 96), bc
        ; bc' now contains cx
        ld bc, (_cx)
    exx
    call _fp_mul
    ld (iy + syz2), hl
    ; cy * z2
    ld hl, (_cy)
    ld (iy - 93), hl
    call _fp_mul
    ld (iy + cyz2), hl
    ; cx * y1
    exx
        ; bc' contains cx
        ld (iy - 93), bc
        ; de' contains y1
        ld (iy - 96), de
        ; de' now contains y2
        ld de, (iy + y2)
    exx
    call _fp_mul
    ld (iy + cxy1), hl
    ; cx * y2
    exx
        ; de' contains y2
        ld (iy - 96), de
        ; set de' to sx
        ; set hl' to -sx
        ld de, (_sx)
        or a, a
        sbc hl, hl
        sbc hl, de
    exx
    call _fp_mul
    ld (iy + cxy2), hl
    ; -sx*y2
    exx
        ; hl' contains -sx
        ld (iy - 93), hl
        ; hl' now contains -171.377608
        ld hl, -1403927
    exx
    call _fp_mul
    ld (iy + nsxy2), hl
    ; Restore the stack pointer (For real this time)
    lea hl, iy - 90
    ld sp, hl
    ; (sx*sum1) + cxy1
    exx
        ; de' contains sx
        push de
    exx
    ld hl, (iy + sum1)
    push hl
    call _fp_mul
    ld de, (iy + cxy1)
    add hl, de
    ld (iy + dy), hl
    call abs
    
    functionEnd:
    ld sp, iy
    pop ix
    ret
section .data
private renderedPoints
renderedPoints = 0
private texture
texture = 48
private objectX
objectX = 49
private objectY
objectY = 51
private objectZ
objectZ = 53
private visible
visible = 55
private outline
outline = 56
private objectPointer
objectPointer = 6
private x1
x1 = -3
private x2
x2 = -6
private y1
y1 = -9
private y2
y2 = -12
private z1
z1 = -15
private z2
z2 = -18
private x1squared
x1squared = -21
private x2squared
x2squared = -24
private y1squared
y1squared = -27
private y2squared
y2squared = -30
private z1squared
z1squared = -33
private z2squared
z2squared = -36
private cxy1
cxy1 = -39
private cxy2
cxy2 = -42
private nsxy1
nsxy1 = -45
private nsxy2
nsxy2 = -48
private ncyx1
ncyx1 = -51
private ncyx2
ncyx2 = -54
private cyz1
cyz1 = -57
private cyz2
cyz2 = -60
private syx1
syx1 = -63
private syx2
syx2 = -66
private syz1
syz1 = -69
private syz2
syz2 = -72
private sum1
sum1 = -75
private sum2
sum2 = -78
private dx
dx = -81
private dy
dy = -84
private dz
dz = -87
extern _cameraXYZ
extern _angleX
extern _cx
extern _sx
extern _cy
extern _sy
extern __fcmp
extern _fp_to_int
extern __imulu_fast
extern approx_sqrt_a
extern _fp_mul
extern abs