// prompt: POSIX read syscall binding with stack, borrow, and overwrite contract.
extern "c" def Stigi.Posix.read(descriptor: I32, buffer: Array[U8](count), count: U64) <externStack(512), borrows(buffer), overwrites(buffer)>: I64
