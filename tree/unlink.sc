// prompt: POSIX unlink syscall removing a null-terminated path buffer.
pub extern "c" def unlink(path: Array[U8](4096)) <externStack(512), borrows(path)>: I32
