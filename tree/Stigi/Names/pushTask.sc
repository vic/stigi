// prompt: push one task on the task stack of a unit walk, failing the walk with a full stack when there is no room.
def Stigi.Names.pushTask {Stigi.Mut} (walker: Stigi.Names.Walker, task: Stigi.Names.Task): Stigi.Names.Walker =
  val Walker(tasks, depth, locals, localCount, edges, references, unresolved, steps, fault) = walker
  if depth < Stigi.Names.maxScope then
    @<below(depth, Stigi.Names.maxScope)>
    tasks(depth) := task
    Walker(tasks, depth + 1, locals, localCount, edges, references, unresolved, steps, fault)
  else Stigi.Names.failWith(walker, StackFull)
