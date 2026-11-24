// prompt: the hash of one unit reference.
def Stigi.Types.referenceHash(unit: Stigi.Core.Reference): U64 =
  unit match
    case ExternalRef(unitSlot, memberSlot) => U64.mix(U64.bitwiseXor(U64.mix(unitSlot), memberSlot))
    case SelfGroupRef(memberSlot) => U64.mix(U64.bitwiseXor(Stigi.Types.hashSeed, memberSlot))
    case InnerRef(memberSlot) => U64.mix(U64.bitwiseXor(U64.mix(Stigi.Types.hashSeed), memberSlot))
