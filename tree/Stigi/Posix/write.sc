// prompt: POSIX write syscall binding with stack and borrow contract.
extern "c" def Stigi.Posix.write(descriptor: I32, buffer: Array[U8](count), count: U64) <externStack(512), borrows(buffer)>: I64
