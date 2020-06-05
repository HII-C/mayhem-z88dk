
; half __sub (half left, half right)

SECTION code_fp_math16

PUBLIC cm16_sdcc_sub

EXTERN asm_f24_f16
EXTERN asm_f16_f24

EXTERN asm_f24_neg
EXTERN asm_f24_add_f24

.cm16_sdcc_sub

    ; subtract sdcc half from sdcc half
    ;
    ; enter : stack = sdcc_half right, sdcc_half left, ret
    ;
    ; exit  :    HL = sdcc_half(left+right)
    ;
    ; uses  : af, bc, de, hl, af', bc', de', hl'

    pop bc                      ; pop return address
    pop hl                      ; get left operand off of the stack
    exx

    pop hl                      ; get right operand off of the stack
    push hl
    exx

    push hl
    push bc                     ; return address on stack
    exx

    call asm_f24_f16            ; expand to dehl
    call asm_f24_neg
    exx                         ; -y    d'  = eeeeeeee e' = s-------
                                ;       hl' = 1mmmmmmm mmmmmmmm
    call asm_f24_f16            ; expand to dehl
                                ; x      d  = eeeeeeee e  = s-------
                                ;        hl = 1mmmmmmm mmmmmmmm
    call asm_f24_add_f24
    jp asm_f16_f24              ; enter stack = sdcc_half right, sdcc_half left, ret
                                ;          HL = sdcc_half right
                                ; return   HL = sdcc_half

