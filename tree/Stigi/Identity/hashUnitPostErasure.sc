// prompt: compute the post-erasure runtime computation BLAKE3 digest for one unit.
pub def Stigi.Identity.hashUnitPostErasure {region: Stigi.Region, Stigi.Mut} (kindTag: U8, arity: U64, bodyLen: U64, bodyBytes: Array[U8](bodyLen), depCap: U64, depDigests: Array[U8](depCap), depBytesLen: U64) <below(bodyLen, 65536), lessOrEqual(depBytesLen, depCap), below(depCap, 65536)>: Stigi.Hash.Digest =
  val zeroByte: U8 = 0
  val totalCap: U64 = 64 + bodyLen + depBytesLen
  if totalCap < 262144 then
    Array.tryFill{region}(totalCap, zeroByte) match
      case FillFailed => Stigi.Hash.Digest.DigestWords(0, 0, 0, 0, 0, 0, 0, 0)
      case Filled(buffer) =>
        val schema: Array[U8](13) = "stigi/unit/v1"
        val offOne: U64 = Stigi.Print.writeSlice(13, schema, 0, 13, totalCap, buffer, 0)
        val offTwo: U64 = if offOne < totalCap then
          buffer(offOne) := kindTag
          offOne + 1
        else
          offOne
        val offThree: U64 = Stigi.Identity.encodeUnsignedLeb(arity, totalCap, buffer, offTwo)
        val offFour: U64 = Stigi.Identity.encodeUnsignedLeb(bodyLen, totalCap, buffer, offThree)
        val offFive: U64 = Stigi.Print.writeSlice(bodyLen, bodyBytes, 0, bodyLen, totalCap, buffer, offFour)
        val offSix: U64 = Stigi.Identity.encodeUnsignedLeb(depBytesLen, totalCap, buffer, offFive)
        val finalLen: U64 = Stigi.Print.writeSlice(depCap, depDigests, 0, depBytesLen, totalCap, buffer, offSix)
        Stigi.Hash.hashBytes{region}(totalCap, buffer, finalLen)
  else
    Stigi.Hash.Digest.DigestWords(0, 0, 0, 0, 0, 0, 0, 0)
