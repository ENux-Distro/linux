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

long sys_read(int fd, char *buf, unsigned long count) {
    return syscall6(0, fd, (long)buf, count, 0, 0, 0); // SYS_read = 0
}

long sys_write(int fd, const char *buf, unsigned long count) {
    return syscall6(1, fd, (long)buf, count, 0, 0, 0); // SYS_write = 1
}

long sys_reboot() {
    return syscall6(169, 0xfee1dead, 672274793, 0xCDEF0123, 0, 0, 0);
}

long sys_pause() {
    return syscall6(34, 0, 0, 0, 0, 0, 0);
}

void sys_exit(int status) {
    syscall6(60, status, 0, 0, 0, 0, 0); // SYS_exit = 60
    while (1);
}

void _start(void) {

    sys_write(1, "Hi\n", 3);

    /*
     * smol.c runs as PID 1 in userspace (ring 3), so privileged
     * instructions like cli/hlt raise #GP -> SIGSEGV -> PID 1 dies ->
     * "Attempted to kill init!" panic. Ask the kernel to halt instead:
     * reboot(LINUX_REBOOT_MAGIC1, LINUX_REBOOT_MAGIC2, LINUX_REBOOT_CMD_HALT)
     */
    sys_reboot(); // SYS_reboot = 169

    /* PID 1 must never exit; if the halt somehow fails, sleep forever. */
    for (;;)
        sys_pause(); // SYS_pause = 34
}
