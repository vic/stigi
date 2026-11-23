// prompt: POSIX getrusage syscall writing one hundred forty-four bytes of rusage.
pub extern "c" def getrusage(whoKind: I32, usageBuf: Array[U8](144)) <externStack(512), borrows(usageBuf), overwrites(usageBuf)>: I32
