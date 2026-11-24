// prompt: verify that each of the five walk faults fails the walk with its own reason and a sound walk keeps its edges.
val Stigi.Names.Tests.reasonSuite {Stigi.Test}: Stigi.Test.Result =
  val code: U64 =
    val Filled(sources) = Array.tryFill[U64]{Stigi.Region}(Stigi.Names.maxEdges, 0) else
      case FillFailed => 99
    val Filled(targets) = Array.tryFill[U64]{Stigi.Region}(Stigi.Names.maxEdges, 0) else
      case FillFailed => 99
    val Filled(stamps) = Array.tryFill[U64]{Stigi.Region}(Stigi.Limit.maxUnits, 0) else
      case FillFailed => 99
    val edges: Stigi.Names.Edges = Edges(sources, targets, 0, stamps)
    val stack: U64 = Stigi.Names.walkVerdict(StackFull, 0, edges) match
      case WalkFailed(WalkFaulted(ReasonStack)) => 0
      case WalkFailed(_) | Walked(_) => 1
    val scope: U64 = Stigi.Names.walkVerdict(ScopeFull, 0, edges) match
      case WalkFailed(WalkFaulted(ReasonScope)) => 0
      case WalkFailed(_) | Walked(_) => 2
    val table: U64 = Stigi.Names.walkVerdict(TableMalformed, 0, edges) match
      case WalkFailed(WalkFaulted(ReasonTable)) => 0
      case WalkFailed(_) | Walked(_) => 3
    val steps: U64 = Stigi.Names.walkVerdict(StepsExhausted, 0, edges) match
      case WalkFailed(WalkFaulted(ReasonSteps)) => 0
      case WalkFailed(_) | Walked(_) => 4
    val full: U64 = Stigi.Names.walkVerdict(EdgesExhausted, 0, edges) match
      case WalkFailed(WalkFaulted(ReasonEdges)) => 0
      case WalkFailed(_) | Walked(_) => 6
    val walked: U64 = Stigi.Names.walkVerdict(WalkSound, 0, edges) match
      case Walked(_) => 0
      case WalkFailed(_) => 5
    if stack != 0 then stack else if scope != 0 then scope else if table != 0 then table else if steps != 0 then steps else if full != 0 then full else walked
  if code == 0 then Stigi.Test.expect(true, true, "each walk fault fails with its own reason and a sound walk is Walked") else if code == 1 then Stigi.Test.expect(false, true, "clause StackFull fails with ReasonStack") else if code == 2 then Stigi.Test.expect(false, true, "clause ScopeFull fails with ReasonScope") else if code == 3 then Stigi.Test.expect(false, true, "clause TableMalformed fails with ReasonTable") else if code == 4 then Stigi.Test.expect(false, true, "clause StepsExhausted fails with ReasonSteps") else if code == 6 then Stigi.Test.expect(false, true, "clause EdgesExhausted fails with ReasonEdges") else if code == 5 then Stigi.Test.expect(false, true, "clause a sound walk with no unresolved reference is Walked") else Stigi.Test.expect(false, true, "clause setup allocates the edge tables")
