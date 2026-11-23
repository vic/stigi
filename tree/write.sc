// prompt: POSIX write syscall with bounded stack and borrowed read-only buffer.
pub extern "c" def write(desc: I32, buf: Array[U8](count), count: U64) <externStack(512), borrows(buf)>: I64
