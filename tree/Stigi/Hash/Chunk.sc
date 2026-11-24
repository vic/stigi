// prompt: one BLAKE3 chunk: its first byte, its index and whether it is the whole input.
enum Stigi.Hash.Chunk:
  case Chunk(start: U64, index: U64, root: Bool)
