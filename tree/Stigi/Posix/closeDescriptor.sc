// prompt: close an open POSIX file descriptor exactly once.
pub def Stigi.Posix.closeDescriptor(descriptor: I32) <lessOrEqual(0, 1)>: Bool =
  Stigi.Posix.close(descriptor) == 0
