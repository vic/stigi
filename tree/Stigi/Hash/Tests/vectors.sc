// prompt: verify official BLAKE3 digests of the empty input, abc, and a five-chunk input through the parent tree.
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
  val longValid: Bool =
    val Filled(longBuf) = Array.tryFill[U8]{Stigi.Region}(4097, 0) else
      case FillFailed => false
    var fillIdx: U64 = 0
    while fillIdx < 4097 do
      @<lessOrEqual(fillIdx, 4097)>
      longBuf(fillIdx) := U8.ofU64(fillIdx % 251)
      fillIdx = fillIdx + 1
    val Hashed(longDigest) = Stigi.Hash.hashBytes{Stigi.Region}(4097, longBuf, 4097) else
      case HashOutOfMemory => false
    val expectedLong: Stigi.Hash.Digest = Digest(U32.ofU64(0xb352409b), U32.ofU64(0xc85f1c8f), U32.ofU64(0x7afff9b1), U32.ofU64(0xd27cb2c7), U32.ofU64(0x3d7b4842), U32.ofU64(0xc9150d89), U32.ofU64(0xb8251c6a), U32.ofU64(0x95b90faa))
    Stigi.sameValue(longDigest, expectedLong)
  Stigi.Test.expect(emptyValid && abcValid && longValid, true, "BLAKE3 empty, abc and five-chunk digests must match the reference")
