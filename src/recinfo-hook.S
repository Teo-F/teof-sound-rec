.text
.p2align 2
.global sound_rec_info_hook
.hidden sound_rec_info_hook
.hidden sound_rec_original_info
.hidden sound_rec_requested_bits
.type sound_rec_info_hook, %function
sound_rec_info_hook:
.cfi_startproc
    stp x29, x30, [sp, #-32]!
    .cfi_def_cfa_offset 32
    .cfi_offset 29, -32
    .cfi_offset 30, -24
    str x19, [sp, #16]
    .cfi_offset 19, -16
    mov x29, sp
    .cfi_def_cfa_register 29
    mov x19, x8
    adrp x16, sound_rec_original_info
    ldr x16, [x16, :lo12:sound_rec_original_info]
    blr x16
    cbz x19, .Ldone
    ldr w9, [x19, #12]
    cmp w9, #16
    b.eq .Lformat
    cmp w9, #24
    b.ne .Ldone
.Lformat:
    ldr w9, [x19, #16]
    cmp w9, #2
    b.ne .Ldone
    ldr w9, [x19, #8]
    mov w10, #44100
    cmp w9, w10
    b.ne .Ldone
    adrp x9, sound_rec_requested_bits
    add x9, x9, :lo12:sound_rec_requested_bits
    ldar w9, [x9]
    cmp w9, #16
    b.eq .Lstore
    cmp w9, #24
    b.ne .Ldone
.Lstore:
    str w9, [x19, #12]
.Ldone:
    ldr x19, [sp, #16]
    ldp x29, x30, [sp], #32
    .cfi_def_cfa sp, 0
    .cfi_restore 19
    .cfi_restore 29
    .cfi_restore 30
    ret
.cfi_endproc
.size sound_rec_info_hook, .-sound_rec_info_hook
.section .note.GNU-stack,"",%progbits
