// prompt: open a verified null-terminated path read-only via POSIX openat.
def Stigi.Posix.openRead(pathLen: U64, path: Array[U8](4096)) <below(pathLen, 4096)>: Stigi.Posix.OpenResult =
  if path(pathLen) == 0 then
    val desc: I32 = openat(-100, path, 0, 0)
    if desc < 0 then OpenFailed else Opened(desc)
  else OpenFailed
