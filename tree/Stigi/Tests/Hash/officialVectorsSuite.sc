// prompt: verify BLAKE3 against official empty, short text, and 1025-byte multi-chunk test vectors.
val Stigi.Tests.Hash.officialVectorsSuite {Stigi.Test}: Stigi.Test.Result =
  val zeroByte: U8 = 0
  val emptyValid: Bool = Array.tryFill{Stigi.Region}(1, zeroByte) match
    case FillFailed => false
    case Filled(emptyBuf) =>
      val gotEmpty: Stigi.Hash.Digest = Stigi.Hash.hashBytes{Stigi.Region}(1, emptyBuf, 0)
      val wantEmpty: Stigi.Hash.Digest = Stigi.Hash.Digest.DigestWords(0xb94913af, 0xa6a1f9f5, 0xea4d40a0, 0x49c9dc36, 0xc925cb9b, 0xb712c1ad, 0xca939acc, 0x62321fe4)
      Stigi.Hash.digestsEqual(gotEmpty, wantEmpty)
  val abcText: Array[U8](3) = "abc"
  val gotAbc: Stigi.Hash.Digest = Stigi.Hash.hashBytes{Stigi.Region}(3, abcText, 3)
  val wantAbc: Stigi.Hash.Digest = Stigi.Hash.Digest.DigestWords(0xacb33764, 0x33514638, 0x753bb6ff, 0xb58d3a27, 0x4658c548, 0x03db795d, 0x6c9c35fd, 0x859dbdd5)
  val abcValid: Bool = Stigi.Hash.digestsEqual(gotAbc, wantAbc) && Stigi.Hash.digestCompare(gotAbc, wantAbc) == 0
  val multiValid: Bool = Array.tryFill{Stigi.Region}(1025, zeroByte) match
    case FillFailed => false
    case Filled(pattern) =>
      var pos: U64 = 0
      while pos < 1025 do
        @<lessOrEqual(pos, 1025), decreases(1025 - pos)>
        val modded: U64 = U64.bitwiseAnd(pos % 251, 255)
        pattern(pos) := U8.ofU64(modded)
        pos = pos + 1
      val gotMulti: Stigi.Hash.Digest = Stigi.Hash.hashBytes{Stigi.Region}(1025, pattern, 1025)
      val wantMulti: Stigi.Hash.Digest = Stigi.Hash.Digest.DigestWords(0xae7802d0, 0xb327eb47, 0x67cfae4f, 0x3f26feb4, 0x2941d582, 0xd9ffc116, 0xfbb78c7c, 0x44844b81)
      Array.tryFill{Stigi.Region}(32, zeroByte) match
        case FillFailed => false
        case Filled(rawBytes) =>
          val wrote: U64 = Stigi.Hash.writeDigestBytes(gotMulti, 32, rawBytes, 0)
          Stigi.Hash.digestsEqual(gotMulti, wantMulti) && wrote == 32 && rawBytes(0) == 0xd0 && rawBytes(31) == 0x44
  Stigi.Test.expect(emptyValid && abcValid && multiValid, true, "BLAKE3 matches official empty, abc, and 1025-byte multi-chunk vectors")
