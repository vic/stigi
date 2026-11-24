// prompt: a name to look up: a text of the current unit, alone or behind a namespace prefix taken from the name pool, or the join of two runs of the unit text.
enum Stigi.Names.Candidate:
  case Bare(first: U64, end: U64) <lessOrEqual(first, end), lessOrEqual(end, Stigi.Limit.maxUnitBytes)>
  case Prefixed(prefixFirst: U64, prefixEnd: U64, first: U64, end: U64) <lessOrEqual(prefixFirst, prefixEnd), lessOrEqual(prefixEnd, Stigi.Limit.maxIndexBytes), lessOrEqual(first, end), lessOrEqual(end, Stigi.Limit.maxUnitBytes)>
  case Joined(targetFirst: U64, targetEnd: U64, tailFirst: U64, tailEnd: U64) <lessOrEqual(targetFirst, targetEnd), lessOrEqual(targetEnd, Stigi.Limit.maxUnitBytes), lessOrEqual(tailFirst, tailEnd), lessOrEqual(tailEnd, Stigi.Limit.maxUnitBytes)>
  case PrefixedJoined(prefixFirst: U64, prefixEnd: U64, targetFirst: U64, targetEnd: U64, tailFirst: U64, tailEnd: U64) <lessOrEqual(prefixFirst, prefixEnd), lessOrEqual(prefixEnd, Stigi.Limit.maxIndexBytes), lessOrEqual(targetFirst, targetEnd), lessOrEqual(targetEnd, Stigi.Limit.maxUnitBytes), lessOrEqual(tailFirst, tailEnd), lessOrEqual(tailEnd, Stigi.Limit.maxUnitBytes)>
