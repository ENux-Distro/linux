/*
init/rescueinit.c - Drop into rescue init shell when no init is found
Usage: syscall
Example:
write(1, "hi\n", 3);
Outputs hi
*/

#define STDIN_FILENO  0
#define STDOUT_FILENO 1

// Raw x86_64 Generic Syscall Wrapper

long syscall6(long num, long a1, long a2, long a3, long a4, long a5, long a6) {
    long ret;
    register long r10 __asm__("r10") = a4;
    register long r8  __asm__("r8")  = a5;
    register long r9  __asm__("r9")  = a6;

    __asm__ volatile (
        "syscall"
        : "=a" (ret)
        : "a" (num), "D" (a1), "S" (a2), "d" (a3), "r" (r10), "r" (r8), "r" (r9) // Fucking hell, pretty understandable (!)
        : "rcx", "r11", "memory"
    );
    return ret;
}

// Convenient helpers for standard REPL operations
long sys_read(int fd, char *buf, unsigned long count) {
    return syscall6(0, fd, (long)buf, count, 0, 0, 0); // SYS_read = 0
}

long sys_write(int fd, const char *buf, unsigned long count) {
    return syscall6(1, fd, (long)buf, count, 0, 0, 0); // SYS_write = 1
}

void sys_exit(int status) {
    syscall6(60, status, 0, 0, 0, 0, 0); // SYS_exit = 60
    while (1);
}

// Syscall Lookup Table

struct syscall_entry {
    const char *name;
    long num;
};

// Expandable table for friendly names
static const struct syscall_entry SYSCALL_TABLE[] = {
    { "read",       0 },
    { "write",      1 },
    { "open",       2 },
    { "close",      3 },
    { "stat",       4 },
    { "fstat",      5 },
    { "lseek",      8 },
    { "mmap",       9 },
    { "mprotect",   10 },
    { "munmap",     11 },
    { "brk",        12 },
    { "ioctl",      16 },
    { "access",     21 },
    { "pipe",       22 },
    { "sched_yield",24 },
    { "dup",        32 },
    { "dup2",       33 },
    { "pause",      34 },
    { "nanosleep",  35 },
    { "getpid",     39 },
    { "fork",       57 },
    { "vfork",      58 },
    { "execve",     59 }, // EXECVE, MY BELOVED
    { "exit",       60 },
    { "wait4",      61 },
    { "kill",       62 },
    { "uname",      63 },
    { "fcntl",      72 },
    { "fsync",      74 },
    { "truncate",   76 },
    { "getcwd",     79 },
    { "chdir",      80 },
    { "mkdir",      83 },
    { "rmdir",      84 },
    { "link",       86 },
    { "unlink",     87 },
    { "readlink",   89 },
    { "getuid",     102 },
    { "getgid",     104 },
    { "setuid",     105 },
    { "setgid",     106 },
    { "geteuid",    107 },
    { "getegid",    108 },
    { "reboot",     169 },
    { "gettid",     186 },
    { 0, 0 }
};

// Freestanding String Utilities

int my_strlen(const char *s) {
    int i = 0;
    while (s[i]) i++;
    return i;
}

void print(const char *s) {
    sys_write(STDOUT_FILENO, s, my_strlen(s));
}

int my_strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

long my_atoi(const char *str) {
    long res = 0;
    int sign = 1;

    while (*str == ' ' || *str == '\t') str++;

    if (*str == '-') {
        sign = -1;
        str++;
    } else if (*str == '+') {
        str++;
    }

    while (*str >= '0' && *str <= '9') {
        res = res * 10 + (*str - '0');
        str++;
    }
    return res * sign;
}

// Convert numbers/errors into printed characters
void print_int(long val) {
    if (val < 0) {
        print("-");
        val = -val;
    }

    char buf[32];
    int i = 0;

    if (val == 0) {
        buf[i++] = '0';
    } else {
        while (val > 0) {
            buf[i++] = '0' + (val % 10);
            val /= 10;
        }
    }

    // Reverse and output string
    char rev[32];
    for (int j = 0; j < i; j++) {
        rev[j] = buf[i - 1 - j];
    }
    rev[i] = '\0';

    print(rev);
}

// Argument Parser


// Written countless shells, still need help on writing argument parsers :)

int parse_args(char *args_str, long *out_args) {
    int count = 0;
    char *p = args_str;

    while (*p && *p != ')' && count < 6) {
        while (*p == ' ' || *p == '\t' || *p == ',') p++;
        if (*p == ')' || *p == '\0' || *p == ';') break;

        if (*p == '"') {
            // String Literal Argument -> pass buffer pointer
            p++; // Skip opening quote
            out_args[count++] = (long)p;

            char *write_ptr = p;
            while (*p && *p != '"') {
                if (*p == '\\' && *(p + 1) == 'n') {
                    *write_ptr++ = '\n';
                    p += 2;
                } else {
                    *write_ptr++ = *p++;
                }
            }
            if (*p == '"') p++;
            *write_ptr = '\0'; // Null terminate string payload
        } else {
            // Integer argument (can be positive/negative)
            out_args[count++] = my_atoi(p);
            while (*p && *p != ',' && *p != ')' && *p != ' ') p++;
        }
    }
    return count;
}

// REPL Command Evaluator

void evaluate_command(char *cmd) {
    // Trim leading whitespace
    while (*cmd == ' ' || *cmd == '\t') cmd++;
    if (*cmd == '\0' || *cmd == '\n') return;

    // Parse function name
    char name_buf[32];
    int i = 0;
    while (cmd[i] && cmd[i] != '(' && cmd[i] != ' ' && i < 31) {
        name_buf[i] = cmd[i];
        i++;
    }
    name_buf[i] = '\0';

    // Parse arguments inside parenthesis
    char *args_start = cmd;
    while (*args_start && *args_start != '(') args_start++;
    if (*args_start == '(') args_start++;

    long args[6] = {0, 0, 0, 0, 0, 0};
    int arg_count = parse_args(args_start, args);

    long sys_num = -1;

    // Check if it's the arbitrary backdoor `sys_raw(num, ...)`
    if (my_strcmp(name_buf, "sys_raw") == 0) {
        if (arg_count < 1) {
            print("Error: sys_raw requires at least a syscall number.\n");
            return;
        }
        sys_num = args[0];
        // Shift arguments left by 1 since arg[0] was the syscall number
        for (int k = 0; k < 5; k++) {
            args[k] = args[k + 1];
        }
        args[5] = 0;
    } else {
        // Look up in named table
        for (int k = 0; SYSCALL_TABLE[k].name != 0; k++) {
            if (my_strcmp(name_buf, SYSCALL_TABLE[k].name) == 0) {
                sys_num = SYSCALL_TABLE[k].num;
                break;
            }
        }
    }

    if (sys_num == -1) {
        print("Error: Unknown syscall '");
        print(name_buf);
        print("'\n");
        return;
    }

    // Execute the raw assembly syscall
    long result = syscall6(sys_num, args[0], args[1], args[2], args[3], args[4], args[5]);

}

// Main REPL Loop (PID 1 Entry Point)

void _start(void) {
    char buf[256];

    print("\nERROR: No init found, dropping into rescue init shell\n");

    for (;;) {
        print("init> ");
        long n = sys_read(STDIN_FILENO, buf, sizeof(buf) - 1);
            if (n <= 0) break;

            buf[n] = '\0'; // Newlines, go kill yourselves

            while (n > 0 && (buf[n - 1] == '\n' || buf[n - 1] == '\r' || buf[n - 1] == ' ')) {
                buf[--n] = '\0';
            }

            evaluate_command(buf);
        }

    sys_exit(0);
}
