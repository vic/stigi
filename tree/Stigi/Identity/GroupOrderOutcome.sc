// prompt: outcome of canonically ordering and hashing a mutually recursive SCC pair.
pub enum Stigi.Identity.GroupOrderOutcome:
  case GroupOrdered(groupDigest: Stigi.Hash.Digest, swapped: Bool)
  case GroupAmbiguous
