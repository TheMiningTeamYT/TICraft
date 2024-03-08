assume adl=1
section .text
; Probably has room for improvement
; Takes a number N
; returns: (N^2) and (N^2 + 40N + 400)
public _findXZSquared
_findXZSquared:
    ; Save the return address to IY
    pop iy
    ; Save the location of the return struct to DE
    pop de
    ; Convert n (in place) to an interger
    call _fp_to_int
    ; Put the interger form of N into BC
    push hl
    pop bc
    ; Multiply N by 40 (the cube size*2)
    ; This code will need to be updated if, in the future, the cube size changes
    add hl, hl
    add hl, hl
    add hl, bc
    add hl, hl
    add hl, hl
    add hl, hl
    ; Save the location of the return struct to the stack
    push de
    ; We need to save this value for later
    push hl
    ; Now square N
    ; Restore HL to BC
    or a, a
    sbc hl, hl
    add hl, bc
    call __imulu_fast
    ; Put the value from before into DE
    pop de
    ; Save the return address to the stack and grab the location of the return struct from the stack
    ex (sp), iy
    ; Save n squared to iy
    ld (iy), hl
    ; Find (n+20) squared
    add hl, de
    ld de, 400
    add hl, de
    ; Save (n+20) squared to iy + 3
    ld (iy + 3), hl
    ; Set HL to IY
    lea hl, iy
    ; Grab the return address from the stack
    ex (sp), iy
    jp (iy)
section .text
public _findYSquared
_findYSquared:
    ; Save the return address to IY
    pop iy
    ; Save the location of the return struct to DE
    pop de
    ; Convert n (in place) to an interger
    call _fp_to_int
    ; Put the interger form of N into BC
    push hl
    pop bc
    ; Multiply N by 40 (the cube size*2)
    ; This code will need to be updated if, in the future, the cube size changes
    add hl, hl
    add hl, hl
    add hl, bc
    add hl, hl
    add hl, hl
    add hl, hl
    ; Save the location of the return struct to the stack
    push de
    ; We need to save this value for later
    push hl
    ; Now square N
    ; Restore HL to BC
    or a, a
    sbc hl, hl
    add hl, bc
    call __imulu_fast
    ; Put the value from before into DE
    pop de
    ; Save the return address to the stack and grab the location of the return struct from the stack
    ex (sp), iy
    ; Save n squared to iy
    ld (iy), hl
    ; Find (n-20) squared
    or a, a
    sbc hl, de
    ld de, 400
    add hl, de
    ; Save (n+20) squared to iy + 3
    ld (iy + 3), hl
    ; Set HL to IY
    lea hl, iy
    ; Grab the return address from the stack
    ex (sp), iy
    jp (iy)
section .data
extern _fp_to_int
extern __imulu_fast