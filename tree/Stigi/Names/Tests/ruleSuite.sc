// prompt: verify that rule atoms and declarations are walked and that a rule unit owes an edge from its relation: the relation of each rule atom counts as a reference, the arguments are not looked up, the rule edge runs from the relation to the rule, and a value declaration counts its declared type.
val Stigi.Names.Tests.ruleSuite {Stigi.Test}: Stigi.Test.Result =
  val code: U64 =
    val Allocated(tables) = Stigi.Syntax.allocate{Stigi.Region}() else
      case AllocationFailed => 99
    val Filled(pool) = Array.tryFill[U8]{Stigi.Region}(Stigi.Limit.maxIndexBytes, 0) else
      case FillFailed => 99
    val Filled(entries) = Array.tryFill[Stigi.Names.Entry]{Stigi.Region}(Stigi.Limit.maxUnits, Entry(0, 0, DefKind, false)) else
      case FillFailed => 99
    val Filled(slots) = Array.tryFill[Stigi.Names.KeySlot]{Stigi.Region}(Stigi.Names.tableSlots, Vacant) else
      case FillFailed => 99
    val Filled(memberPool) = Array.tryFill[U8]{Stigi.Region}(Stigi.Limit.maxIndexBytes, 0) else
      case FillFailed => 99
    val Filled(table) = Array.tryFill[Stigi.Names.Member]{Stigi.Region}(Stigi.Names.maxMembers, Member(0, CaseMember, 0, 0)) else
      case FillFailed => 99
    val Filled(memberSlots) = Array.tryFill[Stigi.Names.KeySlot]{Stigi.Region}(Stigi.Names.tableSlots, Vacant) else
      case FillFailed => 99
    val Filled(tasks) = Array.tryFill[Stigi.Names.Task]{Stigi.Region}(Stigi.Names.maxScope, Restore(0)) else
      case FillFailed => 99
    val Filled(locals) = Array.tryFill[Stigi.Names.Local]{Stigi.Region}(Stigi.Names.maxScope, Local(Text(0, 0), WithoutCapability)) else
      case FillFailed => 99
    val Filled(sources) = Array.tryFill[U64]{Stigi.Region}(Stigi.Names.maxEdges, 0) else
      case FillFailed => 99
    val Filled(targets) = Array.tryFill[U64]{Stigi.Region}(Stigi.Names.maxEdges, 0) else
      case FillFailed => 99
    val Filled(stamps) = Array.tryFill[U64]{Stigi.Region}(Stigi.Limit.maxUnits, 0) else
      case FillFailed => 99
    val names: Array[U8](18) = "A.rel\nA.r\nA.T\nA.v\n"
    val copied: U64 = Stigi.Array.copy[U8](18, names, 0, 18, Stigi.Limit.maxIndexBytes, pool, 0)
    val Added(alpha) = Stigi.Names.addEntry(Image(pool, entries, 0, slots), 0, 5, Shaped(RelationKind, false)) else
      case ImageFull | UnitUnshaped => 99
    val Added(beta) = Stigi.Names.addEntry(alpha, 6, 9, Shaped(RuleKind, false)) else
      case ImageFull | UnitUnshaped => 99
    val Added(gamma) = Stigi.Names.addEntry(beta, 10, 13, Shaped(TypeKind, false)) else
      case ImageFull | UnitUnshaped => 99
    val Added(delta) = Stigi.Names.addEntry(gamma, 14, 17, Shaped(ValKind, false)) else
      case ImageFull | UnitUnshaped => 99
    val members: Stigi.Names.Members = Members(memberPool, 0, table, 0, memberSlots)
    val sample: Array[U8](51) = "// prompt: demo.\nrule def A.r: A.rel(x) = A.rel(x)\n"
    val Parsed(ruleRoot, _, _) = Stigi.Syntax.Parse.parseUnit(tables, 51, sample, 51) else
      case ParseRejected(_, _) | LexRejected(_) => 99
    val extended: Stigi.Names.Edges = Stigi.Names.unitEdges(Edges(sources, targets, 0, stamps), delta, members, tables, 1, ruleRoot)
    tasks(0) := Visit(ruleRoot)
    val Walker(_, _, _, _, ruleWalked, ruleReferences, ruleUnresolved, _, ruleFault) = Stigi.Names.walkFrom(Walker(tasks, 1, locals, 0, extended, 0, 0, 0, WalkSound), delta, members, tables, 1)
    val Edges(ruleSources, ruleTargets, ruleCount, _) = ruleWalked
    val ruleSound: Bool = ruleFault match
      case WalkSound => true
      case StackFull | ScopeFull | TableMalformed | StepsExhausted => false
    val valText: Array[U8](34) = "// prompt: demo.\nval A.v: A.T = 0\n"
    val Parsed(valRoot, _, _) = Stigi.Syntax.Parse.parseUnit(tables, 34, valText, 34) else
      case ParseRejected(_, _) | LexRejected(_) => 99
    tasks(0) := Visit(valRoot)
    val Walker(_, _, _, _, valWalked, valReferences, valUnresolved, _, valFault) = Stigi.Names.walkFrom(Walker(tasks, 1, locals, 0, ruleWalked, 0, 0, 0, WalkSound), delta, members, tables, 3)
    val Edges(valSources, valTargets, valCount, _) = valWalked
    val valSound: Bool = valFault match
      case WalkSound => true
      case StackFull | ScopeFull | TableMalformed | StepsExhausted => false
    if copied == 18 then
      if ruleCount == 2 then
        if ruleSources(0) == 0 && ruleTargets(0) == 1 && ruleSources(1) == 1 && ruleTargets(1) == 0 then
          if ruleReferences == 2 then
            if ruleUnresolved == 0 then
              if ruleSound then
                if valCount == 3 then
                  if valSources(2) == 3 && valTargets(2) == 2 then
                    if valReferences == 1 then
                      if valUnresolved == 0 then
                        if valSound then 0 else 12
                      else 11
                    else 10
                  else 9
                else 8
              else 7
            else 6
          else 5
        else 4
      else 3
    else 1
  if code == 0 then Stigi.Test.expect(true, true, "a rule edge runs from its relation to the rule and rule atoms are walked by their names alone") else if code == 1 then Stigi.Test.expect(false, true, "clause copied names") else if code == 3 then Stigi.Test.expect(false, true, "clause rule edge count is two") else if code == 4 then Stigi.Test.expect(false, true, "clause rule edge endpoints") else if code == 5 then Stigi.Test.expect(false, true, "clause rule references count is two") else if code == 6 then Stigi.Test.expect(false, true, "clause rule unresolved count is zero") else if code == 7 then Stigi.Test.expect(false, true, "clause rule walk ends WalkSound") else if code == 8 then Stigi.Test.expect(false, true, "clause value edge count is three") else if code == 9 then Stigi.Test.expect(false, true, "clause value edge endpoints") else if code == 10 then Stigi.Test.expect(false, true, "clause value references count is one") else if code == 11 then Stigi.Test.expect(false, true, "clause value unresolved count is zero") else if code == 12 then Stigi.Test.expect(false, true, "clause value walk ends WalkSound") else Stigi.Test.expect(false, true, "clause setup of the rule suite")
