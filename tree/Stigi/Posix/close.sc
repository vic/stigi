// prompt: POSIX close syscall binding with stack contract.
extern "c" def Stigi.Posix.close(descriptor: I32) <externStack(256)>: I32
