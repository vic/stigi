// prompt: format a 32-byte BLAKE3 digest as 64 lowercase hexadecimal ASCII characters.
pub def Stigi.Emit.writeHexDigest {region: Stigi.Region, Stigi.Mut} (digest: Stigi.Hash.Digest, outCap: U64, output: Array[U8](outCap), writeOffset: U64) <lessOrEqual(writeOffset, outCap), below(outCap, 0x100000000)>: U64 <lessOrEqual(writeOffset, result), lessOrEqual(result, outCap)> =
  val zeroByte: U8 = 0
  Array.tryFill{region}(32, zeroByte) match
    case FillFailed => writeOffset
    case Filled(rawBytes) =>
      val writtenLen: U64 = Stigi.Hash.writeDigestBytes(digest, 32, rawBytes, 0)
      var index: U64 = 0
      var pos: U64 = writeOffset
      while index < writtenLen do
        @<lessOrEqual(index, writtenLen), lessOrEqual(writeOffset, pos), lessOrEqual(pos, outCap), decreases(writtenLen - index)>
        val wordVal: U64 = U64.ofU8(rawBytes(index))
        val highNib: U64 = U64.bitwiseAnd(U64.shiftRight(wordVal, 4), 15)
        val lowNib: U64 = U64.bitwiseAnd(wordVal, 15)
        pos = Stigi.Emit.writeByte(outCap, output, pos, Stigi.Emit.hexNibbleByte(highNib))
        pos = Stigi.Emit.writeByte(outCap, output, pos, Stigi.Emit.hexNibbleByte(lowNib))
        index = index + 1
      pos
