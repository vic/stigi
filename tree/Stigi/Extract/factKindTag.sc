// prompt: map a fact kind case to its stable columnar tag byte.
pub def Stigi.Extract.factKindTag(kind: Stigi.Extract.FactKind) <lessOrEqual(0, 1)>: U8 =
  kind match
    case FactBelow => 1
    case FactLessOrEqual => 2
    case FactTermOffset => 3
    case FactAddNode => 4
    case FactSubtractNode => 5
    case FactDominates => 6
    case FactPhiNode => 7
    case FactIndexRead => 8
    case FactIndexWrite => 9
    case FactDivisorNonZero => 10
    case FactCapabilityState => 11
    case FactDenial => 12
    case FactDecreases => 13
