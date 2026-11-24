// prompt: push one elaboration work item onto the work stack and give back the scratch with the new top.
def Stigi.Elaborate.pushWork {Stigi.Mut} (scratch: Stigi.Elaborate.Scratch, item: Stigi.Elaborate.Work): Stigi.Elaborate.PushResult =
  scratch match
    case WorkScratch(capacity, work, top, mapped, patRes) =>
      if top < capacity then
        @<below(top, capacity)>
        val nextTop: U64 = top + 1
        work(top) := item
        PushDone(WorkScratch(capacity, work, nextTop, mapped, patRes))
      else PushFull
