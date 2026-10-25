.text
.p2align 2
.global invoke_info_hook
.type invoke_info_hook, %function
invoke_info_hook:
.cfi_startproc
    stp x29, x30, [sp, #-32]!
    .cfi_def_cfa_offset 32
    .cfi_offset 29, -32
    .cfi_offset 30, -24
    str x19, [sp, #16]
    .cfi_offset 19, -16
    mov x29, sp
    .cfi_def_cfa_register 29
    mov x8, x0
    mov x0, #0xac0
    mov x1, #0xac1
    mov x2, #0xac2
    mov x3, #0xac3
    mov x4, #0xac4
    mov x5, #0xac5
    mov x6, #0xac6
    mov x7, #0xac7
    mov x19, #0x919
    bl sound_rec_info_hook
    cmp x19, #0x919
    b.eq .Lreturn
    mov x0, #0
.Lreturn:
    ldr x19, [sp, #16]
    ldp x29, x30, [sp], #32
    .cfi_def_cfa sp, 0
    .cfi_restore 19
    .cfi_restore 29
    .cfi_restore 30
    ret
.cfi_endproc
.size invoke_info_hook, .-invoke_info_hook
.global original_fixture
.type original_fixture, %function
original_fixture:
    adrp x9, original_calls
    ldr w10, [x9, :lo12:original_calls]
    add w10, w10, #1
    str w10, [x9, :lo12:original_calls]
    cmp x0, #0xac0
    b.ne .Lbad
    cmp x1, #0xac1
    b.ne .Lbad
    cmp x2, #0xac2
    b.ne .Lbad
    cmp x3, #0xac3
    b.ne .Lbad
    cmp x4, #0xac4
    b.ne .Lbad
    cmp x5, #0xac5
    b.ne .Lbad
    cmp x6, #0xac6
    b.ne .Lbad
    cmp x7, #0xac7
    b.ne .Lbad
    cbz x8, .Lresult
    adrp x9, fixture_fields
    add x9, x9, :lo12:fixture_fields
    ldr w10, [x9]
    str w10, [x8, #8]
    ldr w10, [x9, #4]
    str w10, [x8, #12]
    ldr w10, [x9, #8]
    str w10, [x8, #16]
.Lresult:
    mov x8, #0
    mov x0, #0xccdd
    movk x0, #0xaabb, lsl #16
    ret
.Lbad:
    mov x0, #0
    ret
.size original_fixture, .-original_fixture
.section .note.GNU-stack,"",%progbits
