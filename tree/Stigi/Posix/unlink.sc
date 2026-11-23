// prompt: POSIX unlink syscall binding with stack and borrow contract.
extern "c" def Stigi.Posix.unlink(pathBytes: Array[U8](512)) <externStack(512), borrows(pathBytes)>: I32
