// prompt: map a stored unit kind enum to its compact columnar byte tag.
pub def Stigi.Store.unitKindCode(kind: Stigi.Store.UnitKind) <lessOrEqual(0, 1)>: U8 =
  kind match
    case KindEnum => 1
    case KindType => 2
    case KindDef => 3
    case KindLemma => 4
    case KindVal => 5
    case KindCapability => 6
    case KindHandler => 7
    case KindExtern => 8
    case KindRelation => 9
    case KindRule => 10
    case KindTest => 11
    case KindNegTest => 12
    case KindUnknown => 0
