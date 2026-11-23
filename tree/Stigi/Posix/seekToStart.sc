// prompt: seek an open descriptor back to offset zero.
def Stigi.Posix.seekToStart(descriptor: I32) <lessOrEqual(0, 1)>: Bool =
  Stigi.Posix.lseek(descriptor, 0, 0) == 0
