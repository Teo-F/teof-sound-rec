.text
.p2align 2
.global invoke_test_hook
.type invoke_test_hook, %function
invoke_test_hook:
    stp x19, x20, [sp, #-32]!
    stp x21, x30, [sp, #16]
    mov x20, x2
    mov x2, x0
    mov x30, x1
    adr x19, .Lreturned
    mov x0, #0x10
    mov x1, #0x11
    mov x3, #0x13
    mov x4, #0x14
    mov x5, #0x15
    mov x6, #0x16
    mov x7, #0x17
    mov x8, #0x18
    b sound_rec_start_hook
.Lreturned:
    ldp x21, x30, [sp, #16]
    ldp x19, x20, [sp], #32
    ret
.size invoke_test_hook, .-invoke_test_hook
.global test_original
.type test_original, %function
test_original:
    stp x0, x1, [x20]
    stp x2, x3, [x20, #16]
    stp x4, x5, [x20, #32]
    stp x6, x7, [x20, #48]
    stp x8, x30, [x20, #64]
    ret x19
.size test_original, .-test_original
.section .note.GNU-stack,"",%progbits
