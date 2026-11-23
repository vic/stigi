// prompt: outcome of computing a BLAKE3 digest over a proved byte slice.
pub enum Stigi.Hash.HashOutcome:
  case Hashed(digest: Stigi.Hash.Digest)
  case HashOutOfMemory
