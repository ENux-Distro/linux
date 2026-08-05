/*
 * Deliberately NOT .init.rodata: that section is reclaimed and poisoned
 * with 0xcc (POISON_FREE_INITMEM) by free_initmem(), which runs before
 * kernel_init() reaches the "no init found" fallback path that extracts
 * this blob. Keep it in ordinary .rodata so it survives until then.
 */
.section .rodata, "a"
    .global rescueinit_bin_start
    .global rescueinit_bin_end

    .align 8
rescueinit_bin_start:
    .incbin "init/rescueinit_bin"
rescueinit_bin_end:
