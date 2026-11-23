// prompt: POSIX read syscall with bounded stack and borrowed overwritten buffer.
pub extern "c" def read(desc: I32, buf: Array[U8](count), count: U64) <externStack(512), borrows(buf), overwrites(buf)>: I64
