// prompt: stop the walk of a unit with a fault, unless an earlier fault already stopped it.
def Stigi.Names.failWith(walker: Stigi.Names.Walker, cause: Stigi.Names.WalkFault): Stigi.Names.Walker =
  val Walker(tasks, depth, locals, localCount, edges, references, unresolved, steps, fault) = walker
  fault match
    case WalkSound =>
      @<lessOrEqual(steps, Stigi.Names.maxWalkSteps)>
      Walker(tasks, depth, locals, localCount, edges, references, unresolved, steps, cause)
    case StackFull | ScopeFull | TableMalformed | StepsExhausted | EdgesExhausted => walker
