// prompt: remove a verified null-terminated filesystem path via POSIX unlink.
def Stigi.Posix.removePath(pathLen: U64, path: Array[U8](4096)) <below(pathLen, 4096)>: Bool =
  if path(pathLen) == 0 then unlink(path) == 0 else false
