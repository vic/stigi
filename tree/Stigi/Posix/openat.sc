// prompt: POSIX openat syscall binding with stack and borrow contract.
extern "c" def Stigi.Posix.openat(dirDescriptor: I32, pathBytes: Array[U8](512), openFlags: I32, openMode: U32) <externStack(512), borrows(pathBytes)>: I32
