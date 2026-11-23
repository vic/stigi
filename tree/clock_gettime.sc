// prompt: POSIX clock_gettime syscall writing sixteen bytes of timespec.
pub extern "c" def clock_gettime(clockKind: I32, specBuf: Array[U8](16)) <externStack(512), borrows(specBuf), overwrites(specBuf)>: I32
