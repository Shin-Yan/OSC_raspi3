.global memzero
memzero:
    str xzr, [x0], #8
    subs x1, x1, #8
    b.gt memzero
    ret

// dest x0, src x1, len x2
.globl memncpy
memncpy:
    ldr x3, [x1], #8
    str x3, [x0], #8
    subs x2, x2, #8
    b.gt memncpy
    ret