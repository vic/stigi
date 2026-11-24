// prompt: the BLAKE3 tree walk position: next input byte, next chunk index and parent stack depth.
enum Stigi.Hash.Tree:
  case Tree(pos: U64, chunkIndex: U64, depth: U64)
