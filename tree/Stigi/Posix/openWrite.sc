// prompt: create or truncate a verified null-terminated path write-only via POSIX openat.
def Stigi.Posix.openWrite(pathLen: U64, path: Array[U8](4096)) <below(pathLen, 4096)>: Stigi.Posix.OpenResult =
  if path(pathLen) == 0 then
    val desc: I32 = openat(-100, path, 577, 420)
    if desc < 0 then OpenFailed else Opened(desc)
  else OpenFailed
