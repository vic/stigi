// prompt: POSIX lseek syscall binding with stack contract.
extern "c" def Stigi.Posix.lseek(descriptor: I32, offset: I64, whence: I32) <externStack(256)>: I64
