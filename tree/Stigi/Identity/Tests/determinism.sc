// prompt: verify content identity digests are deterministic and sensitive to single-byte edits.
val Stigi.Identity.Tests.determinism {Stigi.Test}: Stigi.Test.Result =
  val textFirst: Array[U8](6) = "alpha!"
  val textSame: Array[U8](6) = "alpha!"
  val textDiff: Array[U8](6) = "alpha?"
  val Hashed(digFirst) = Stigi.Identity.unitDigest{Stigi.Region}(6, textFirst, 6) else
    case HashOutOfMemory => Stigi.Test.expect(false, true, "first digest allocation failed")
  val Hashed(digSame) = Stigi.Identity.unitDigest{Stigi.Region}(6, textSame, 6) else
    case HashOutOfMemory => Stigi.Test.expect(false, true, "second digest allocation failed")
  val Hashed(digDiff) = Stigi.Identity.unitDigest{Stigi.Region}(6, textDiff, 6) else
    case HashOutOfMemory => Stigi.Test.expect(false, true, "third digest allocation failed")
  val firstVal: Stigi.Hash.Digest = digFirst
  val checksPass: Bool = Stigi.sameValue(firstVal, digSame) && !Stigi.sameValue(firstVal, digDiff)
  Stigi.Test.expect(checksPass, true, "content identity must be deterministic and collision-sensitive")
