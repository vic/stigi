// prompt: take one step of a unit walk: count the step first, run the newest task, or fail the walk when it ran out of steps.
def Stigi.Names.stepWalk {Stigi.Mut} (walker: Stigi.Names.Walker, image: Stigi.Names.Image, members: Stigi.Names.Members, tables: Stigi.Syntax.Tables, from: U64) <below(from, Stigi.Limit.maxUnits)>: Stigi.Names.Walker =
  val Walker(tasks, depth, locals, localCount, edges, references, unresolved, steps, fault) = walker
  if depth < 1 then walker else
    if steps < Stigi.Names.maxWalkSteps then
      Stigi.Names.finishStep(Stigi.Names.runTask(Walker(tasks, depth, locals, localCount, edges, references, unresolved, steps + 1, fault), image, members, tables, from), references, unresolved, steps)
    else Stigi.Names.failWith(walker, StepsExhausted)
