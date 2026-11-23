// prompt: POSIX lseek syscall moving a file descriptor offset.
pub extern "c" def lseek(desc: I32, offset: I64, whence: I32) <externStack(256), externSummary()>: I64
