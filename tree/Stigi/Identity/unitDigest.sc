// prompt: compute a canonical BLAKE3 content identity digest for a unit byte slice.
def Stigi.Identity.unitDigest {region: Stigi.Region, Stigi.Mut} (cap: Stigi.ByteCap, source: Array[U8](cap), length: U64) <lessOrEqual(length, cap)>: Stigi.Hash.HashOutcome =
  Stigi.Hash.hashBytes{region}(cap, source, length)
