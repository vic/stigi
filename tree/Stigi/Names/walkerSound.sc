// prompt: whether the walk of a unit has met no fault so far.
def Stigi.Names.walkerSound(walker: Stigi.Names.Walker): Bool =
  val Walker(_, _, _, _, _, _, _, _, fault) = walker
  fault match
    case WalkSound => true
    case StackFull | ScopeFull | TableMalformed | StepsExhausted => false
