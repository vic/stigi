// prompt: the FNV-1a hash of a run of bytes, folding ASCII capitals to small letters when asked.
def Stigi.Names.textHash(cap: U64, bytes: Array[U8](cap), first: U64, end: U64, fold: Bool, acc: U64, remaining: U64) <lessOrEqual(end, cap), decreases(remaining)>: U64 =
  if remaining < 1 || end <= first then acc else
    val byte: U8 = bytes(first)
    val code: U64 = U64.ofU8(byte)
    val folded: U64 = if fold && 'A' <= byte && byte <= 'Z' then code + Stigi.Names.foldStep else code
    Stigi.Names.textHash(cap, bytes, first + 1, end, fold, Stigi.Names.hashStep(acc, folded), remaining - 1)
