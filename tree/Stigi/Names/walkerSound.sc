// prompt: whether the walk of a unit has met no fault so far.
def Stigi.Names.walkerSound(walker: Stigi.Names.Walker): Bool =
  val Walker(_, _, _, _, _, _, _, _, fault) = walker
  fault is WalkSound
