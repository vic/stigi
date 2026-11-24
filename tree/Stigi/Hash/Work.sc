// prompt: the BLAKE3 working arrays of one hash: chaining words, message block, compression state, message copy, permutation scratch, parent stack and padded block bytes.
enum Stigi.Hash.Work:
  case Work(chain: Array[U32](8), block: Array[U32](16), state: Array[U32](16), msg: Array[U32](16), scratch: Array[U32](16), stack: Array[U32](512), padded: Array[U8](64))
