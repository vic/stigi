// prompt: POSIX clock_gettime syscall binding with stack, borrow, and overwrite contract.
extern "c" def Stigi.Posix.clock_gettime(clockKind: I32, timeBytes: Array[U8](16)) <externStack(512), borrows(timeBytes), overwrites(timeBytes)>: I32
