// prompt: verify that a full edge table reports the full result instead of dropping the edge, that a new edge is kept once and a repeat of it leaves the count unchanged, and that a walk recording an edge into a full table stops with the edges fault.
val Stigi.Names.Tests.edgesFullSuite {Stigi.Test}: Stigi.Test.Result =
  val code: U64 =
    val Filled(sources) = Array.tryFill[U64]{Stigi.Region}(Stigi.Names.maxEdges, 0) else
      case FillFailed => 99
    val Filled(targets) = Array.tryFill[U64]{Stigi.Region}(Stigi.Names.maxEdges, 0) else
      case FillFailed => 99
    val Filled(stamps) = Array.tryFill[U64]{Stigi.Region}(Stigi.Limit.maxUnits, 0) else
      case FillFailed => 99
    val Filled(tasks) = Array.tryFill[Stigi.Names.Task]{Stigi.Region}(Stigi.Names.maxScope, Restore(0)) else
      case FillFailed => 99
    val Filled(locals) = Array.tryFill[Stigi.Names.Local]{Stigi.Region}(Stigi.Names.maxScope, Local(Text(0, 0), WithoutCapability)) else
      case FillFailed => 99
    val packed: U64 = Stigi.Names.addEdge(Edges(sources, targets, Stigi.Names.maxEdges, stamps), 0, 1) match
      case EdgesFull => 0
      case EdgeKept(_) => 1
    val kept: U64 = Stigi.Names.addEdge(Edges(sources, targets, 0, stamps), 0, 1) match
      case EdgesFull => 2
      case EdgeKept(once) =>
        val Edges(_, _, onceCount, _) = once
        Stigi.Names.addEdge(once, 0, 1) match
          case EdgesFull => 3
          case EdgeKept(again) =>
            val Edges(_, _, twiceCount, _) = again
            if onceCount == 1 && twiceCount == 1 then 0 else 4
    val Walker(_, _, _, _, _, _, _, _, stopped) = Stigi.Names.walkEdge(Walker(tasks, 0, locals, 0, Edges(sources, targets, Stigi.Names.maxEdges, stamps), 0, 0, 0, WalkSound), 0, 2)
    val walked: U64 = if stopped is EdgesExhausted then 0 else 5
    if packed != 0 then packed else if kept != 0 then kept else walked
  if code == 0 then Stigi.Test.expect(true, true, "a full edge table reports EdgesFull, a repeated edge keeps the count and a walk into a full table stops with the edges fault") else if code == 1 then Stigi.Test.expect(false, true, "clause a full edge table reports EdgesFull") else if code == 2 then Stigi.Test.expect(false, true, "clause a new edge into an empty table is kept") else if code == 3 then Stigi.Test.expect(false, true, "clause a repeated edge is kept") else if code == 4 then Stigi.Test.expect(false, true, "clause a kept edge and its repeat both leave the count at one") else if code == 5 then Stigi.Test.expect(false, true, "clause a walk into a full edge table stops with EdgesExhausted") else Stigi.Test.expect(false, true, "clause setup allocates the edge, task and local tables")
