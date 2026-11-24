// prompt: close one step of a unit walk: keep the tasks, locals, edges and fault the step left, and apply the step count and the counters the step moved to the counts before it.
def Stigi.Names.finishStep(ran: Stigi.Names.Expanded, references: U64, unresolved: U64, before: U64) <lessOrEqual(references, before), lessOrEqual(unresolved, before), below(before, Stigi.Names.maxWalkSteps)>: Stigi.Names.Walker =
  val Expanded(walker, counted) = ran
  val Walker(tasks, depth, locals, localCount, edges, _, _, _, fault) = walker
  counted match
    case NothingCounted => Walker(tasks, depth, locals, localCount, edges, references, unresolved, before + 1, fault)
    case ReferenceCounted => Walker(tasks, depth, locals, localCount, edges, references + 1, unresolved, before + 1, fault)
    case UnresolvedReferenceCounted => Walker(tasks, depth, locals, localCount, edges, references + 1, unresolved + 1, before + 1, fault)
