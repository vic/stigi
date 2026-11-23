// prompt: POSIX close syscall releasing a file descriptor once.
pub extern "c" def close(desc: I32) <externStack(256), externSummary()>: I32
