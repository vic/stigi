// prompt: POSIX getrusage syscall binding with stack, borrow, and overwrite contract.
extern "c" def Stigi.Posix.getrusage(who: I32, usageBytes: Array[U8](144)) <externStack(512), borrows(usageBytes), overwrites(usageBytes)>: I32
