// prompt: verify official BLAKE3 test vectors and identity equality via uKanren.
val Stigi.Hash.Tests.vectors {Stigi.Test}: Stigi.Test.Result =
  Stigi.Hash.Laws.scheduleRange(7)
  val emptyValid: Bool =
    val Filled(emptyBuf) = Array.tryFill[U8]{Stigi.Region}(1, 0) else
      case FillFailed => false
    val Hashed(emptyDigest) = Stigi.Hash.hashBytes{Stigi.Region}(1, emptyBuf, 0) else
      case HashOutOfMemory => false
    val expectedEmpty: Stigi.Hash.Digest = Digest(U32.ofU64(0xb94913af), U32.ofU64(0xa6a1f9f5), U32.ofU64(0xea4d40a0), U32.ofU64(0x49c9dc36), U32.ofU64(0xc925cb9b), U32.ofU64(0xb712c1ad), U32.ofU64(0xca939acc), U32.ofU64(0x62321fe4))
    Stigi.sameValue(emptyDigest, expectedEmpty)
  val abcValid: Bool =
    val abcBytes: Array[U8](3) = "abc"
    val Hashed(abcDigest) = Stigi.Hash.hashBytes{Stigi.Region}(3, abcBytes, 3) else
      case HashOutOfMemory => false
    val expectedAbc: Stigi.Hash.Digest = Digest(U32.ofU64(0xacb33764), U32.ofU64(0x33514638), U32.ofU64(0x753bb6ff), U32.ofU64(0xb58d3a27), U32.ofU64(0x4658c548), U32.ofU64(0x03db795d), U32.ofU64(0x6c9c35fd), U32.ofU64(0x859dbdd5))
    Stigi.sameValue(abcDigest, expectedAbc)
  Stigi.Test.expect(emptyValid && abcValid, true, "BLAKE3 empty and abc digests must match official vectors")
