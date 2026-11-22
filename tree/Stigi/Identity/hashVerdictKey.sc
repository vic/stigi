// prompt: compute the pre-erasure verdict cache key combining runtime digest, rules, budget, and facts.
pub def Stigi.Identity.hashVerdictKey {region: Stigi.Region, Stigi.Mut} (runtimeDigest: Stigi.Hash.Digest, rulesDigest: Stigi.Hash.Digest, rowBudget: U64, factsLen: U64, factsBytes: Array[U8](factsLen)) <below(factsLen, 65536)>: Stigi.Hash.Digest =
  val zeroByte: U8 = 0
  val totalCap: U64 = 128 + factsLen
  if totalCap < 262144 then
    Array.tryFill{region}(totalCap, zeroByte) match
      case FillFailed => Stigi.Hash.Digest.DigestWords(0, 0, 0, 0, 0, 0, 0, 0)
      case Filled(buffer) =>
        val schema: Array[U8](16) = "stigi/verdict/v1"
        val offOne: U64 = Stigi.Print.writeSlice(16, schema, 0, 16, totalCap, buffer, 0)
        val offTwo: U64 = Stigi.Hash.writeDigestBytes(runtimeDigest, totalCap, buffer, offOne)
        val offThree: U64 = Stigi.Hash.writeDigestBytes(rulesDigest, totalCap, buffer, offTwo)
        val offFour: U64 = Stigi.Identity.encodeUnsignedLeb(rowBudget, totalCap, buffer, offThree)
        val offFive: U64 = Stigi.Identity.encodeUnsignedLeb(factsLen, totalCap, buffer, offFour)
        val finalLen: U64 = Stigi.Print.writeSlice(factsLen, factsBytes, 0, factsLen, totalCap, buffer, offFive)
        Stigi.Hash.hashBytes{region}(totalCap, buffer, finalLen)
  else
    Stigi.Hash.Digest.DigestWords(0, 0, 0, 0, 0, 0, 0, 0)
