// prompt: open a verified null-terminated path via POSIX openat with open flags.
def Stigi.Posix.openPath(pathLen: U64, path: Array[U8](4096), flags: I32) <below(pathLen, 4096)>: Stigi.Posix.OpenResult =
  if path(pathLen) == '\0' then
    val desc: I32 = openat(Stigi.Posix.currentDirectory, path, flags, Stigi.Posix.createdFileMode)
    if desc < 0 then OpenFailed else Opened(desc)
  else OpenFailed
