// prompt: POSIX openat syscall with bounded stack and borrowed path buffer.
pub extern "c" def openat(dirDesc: I32, path: Array[U8](4096), flags: I32, mode: U32) <externStack(512), borrows(path)>: I32
