// prompt: the number of tasks waiting on the task stack of a unit walk.
def Stigi.Names.walkerDepth(walker: Stigi.Names.Walker): U64 =
  val Walker(_, depth, _, _, _, _, _, _, _) = walker
  depth
