.text
.p2align 2
.global sound_rec_start_hook
.hidden sound_rec_start_hook
.type sound_rec_start_hook, %function
sound_rec_start_hook:
    movz x9, #0x11b8
    movk x9, #0xd6, lsl #16
    cmp x30, x9
    b.ne .Lforward
    cbz x2, .Lforward
    ldr w9, [x2, #8]
    mov w10, #44100
    cmp w9, w10
    b.ne .Lforward
    ldr w9, [x2, #16]
    cmp w9, #2
    b.ne .Lforward
    ldr w9, [x2, #12]
    cmp w9, #16
    b.eq .Lselect
    cmp w9, #24
    b.ne .Lforward
.Lselect:
    adrp x9, :got:sound_rec_requested_bits
    ldr x9, [x9, :got_lo12:sound_rec_requested_bits]
    ldar w10, [x9]
    cmp w10, #16
    b.eq .Lstore
    cmp w10, #24
    b.ne .Lforward
.Lstore:
    str w10, [x2, #12]
.Lforward:
    adrp x16, :got:sound_rec_original_start
    ldr x16, [x16, :got_lo12:sound_rec_original_start]
    ldr x16, [x16]
    br x16
.size sound_rec_start_hook, .-sound_rec_start_hook
.section .note.GNU-stack,"",%progbits
