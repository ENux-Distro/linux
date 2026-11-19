/*
Embed a custom, statically-linked, freestanding program of your choice
to the kernel, by editing init/smol.c.
You can add anything you like to the kernel, like a shell, CPU halter
or even a kernel panic program, just make sure that you write code
compatible with your kernel configurations.

NOTE: This smol.c won't be used to contribute to the official Linux
kernel, it's a simple experiment of mine.

Let me help you with building your own init/smol.c. Here's a simple syscall6 implementation

long syscall6(long num, long a1, long a2, long a3, long a4, long a5, long a6) {
    long ret;
    register long r10 __asm__("r10") = a4;
    register long r8  __asm__("r8")  = a5;
    register long r9  __asm__("r9")  = a6;

    __asm__ volatile (
        "syscall"
        : "=a" (ret)
        : "a" (num), "D" (a1), "S" (a2), "d" (a3), "r" (r10), "r" (r8), "r" (r9)
        : "rcx", "r11", "memory"
    );
    return ret;
}

*/
