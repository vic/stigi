// prompt: canonically order and hash a mutually recursive SCC pair, rejecting symmetric shapes as ambiguous.
pub def Stigi.Identity.hashGroupPair {region: Stigi.Region, Stigi.Mut} (leftLen: U64, leftShape: Array[U8](leftLen), rightLen: U64, rightShape: Array[U8](rightLen)) <below(leftLen, 65536), below(rightLen, 65536)>: Stigi.Identity.GroupOrderOutcome =
  val order: I32 = Stigi.Identity.compareSlices(leftLen, leftShape, rightLen, rightShape)
  if order == 0 then
    Stigi.Identity.GroupOrderOutcome.GroupAmbiguous
  else
    val zeroByte: U8 = 0
    val totalCap: U64 = 64 + leftLen + rightLen
    if totalCap < 262144 then
      Array.tryFill{region}(totalCap, zeroByte) match
        case FillFailed => Stigi.Identity.GroupOrderOutcome.GroupAmbiguous
        case Filled(buffer) =>
          val schema: Array[U8](14) = "stigi/group/v1"
          val offOne: U64 = Stigi.Print.writeSlice(14, schema, 0, 14, totalCap, buffer, 0)
          if order < 0 then
            val offTwo: U64 = Stigi.Identity.encodeUnsignedLeb(leftLen, totalCap, buffer, offOne)
            val offThree: U64 = Stigi.Print.writeSlice(leftLen, leftShape, 0, leftLen, totalCap, buffer, offTwo)
            val offFour: U64 = Stigi.Identity.encodeUnsignedLeb(rightLen, totalCap, buffer, offThree)
            val finalLen: U64 = Stigi.Print.writeSlice(rightLen, rightShape, 0, rightLen, totalCap, buffer, offFour)
            val digest: Stigi.Hash.Digest = Stigi.Hash.hashBytes{region}(totalCap, buffer, finalLen)
            Stigi.Identity.GroupOrderOutcome.GroupOrdered(digest, false)
          else
            val offTwo: U64 = Stigi.Identity.encodeUnsignedLeb(rightLen, totalCap, buffer, offOne)
            val offThree: U64 = Stigi.Print.writeSlice(rightLen, rightShape, 0, rightLen, totalCap, buffer, offTwo)
            val offFour: U64 = Stigi.Identity.encodeUnsignedLeb(leftLen, totalCap, buffer, offThree)
            val finalLen: U64 = Stigi.Print.writeSlice(leftLen, leftShape, 0, leftLen, totalCap, buffer, offFour)
            val digest: Stigi.Hash.Digest = Stigi.Hash.hashBytes{region}(totalCap, buffer, finalLen)
            Stigi.Identity.GroupOrderOutcome.GroupOrdered(digest, true)
    else
      Stigi.Identity.GroupOrderOutcome.GroupAmbiguous
