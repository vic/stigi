// prompt: verify that a fact of the pre-group names a parameter of the same head, as the seed binds all parameter names before it visits the pre-group.
val Stigi.Names.Tests.preGroupSuite {Stigi.Test}: Stigi.Test.Result =
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
    val names: Array[U8](14) = "A.c\nA.rel\nA.T\n"
    val copied: U64 = Stigi.Array.copy[U8](14, names, 0, 14, Stigi.Limit.maxIndexBytes, pool, 0)
    val Added(alpha) = Stigi.Names.addEntry(Image(pool, entries, 0, slots), 0, 3, Shaped(DefKind, false)) else
      case ImageFull | UnitUnshaped => 99
    val Added(beta) = Stigi.Names.addEntry(alpha, 4, 9, Shaped(RelationKind, false)) else
      case ImageFull | UnitUnshaped => 99
    val Added(gamma) = Stigi.Names.addEntry(beta, 10, 13, Shaped(TypeKind, false)) else
      case ImageFull | UnitUnshaped => 99
    val sample: Array[U8](56) = "// prompt: demo.\ndef A.c <A.rel(x)> (x: A.T): A.T =\n  x\n"
    val Parsed(root, _, _) = Stigi.Syntax.Parse.parseUnit(tables, 56, sample, 56) else
      case ParseRejected(_, _) | LexRejected(_) => 99
    val Fetched(rootNode) = Stigi.Syntax.readSlot(tables, root) else
      case MalformedTable(_) => 99
    val UnitRoot(_, _, _, decl) = rootNode else
      case Absent(_) | Name(_, _) | TypeRef(_, _, _, _, _) | FactAtom(_, _, _, _, _) | FactCompare(_, _, _, _, _, _) | FactArith(_, _, _, _, _, _, _) | FactDeny(_, _) | FactGroup(_, _) | Param(_, _, _) | CapParam(_, _, _) | CapGroup(_, _) | PatName(_, _) | PatLiteral(_, _) | PatCtor(_, _, _) | PatTuple(_, _) | PatOr(_, _) | PatRange(_, _, _) | Literal(_, _, _) | Ref(_, _) | Call(_, _, _, _, _, _) | Binary(_, _, _, _) | Unary(_, _, _) | IsTest(_, _, _) | IfElse(_, _, _, _) | WhileLoop(_, _, _) | MatchOn(_, _, _) | Arm(_, _, _, _) | ValStmt(_, _, _, _, _) | VarStmt(_, _, _, _, _) | AssignStmt(_, _, _) | IndexRead(_, _, _) | IndexWrite(_, _, _, _) | HandleStmt(_, _, _, _, _, _, _, _) | AssertStmt(_, _, _) | DenyStmt(_, _) | Block(_, _) | LocalDecl(_, _) | DefHead(_, _, _, _, _, _, _, _) | EnumDecl(_, _, _, _, _) | EnumCase(_, _, _, _, _) | TypeDecl(_, _, _, _, _, _, _, _) | CapabilityDecl(_, _, _, _, _, _, _, _) | HandlerDecl(_, _, _, _, _, _, _, _, _, _) | TraitDecl(_, _, _, _, _) | GivenDecl(_, _, _, _, _, _, _) | ExternDef(_, _, _, _, _, _) | ExternVal(_, _, _, _, _) | RelationDecl(_, _, _, _) | RuleDecl(_, _, _, _, _, _) | FunctionDecl(_, _, _, _, _, _, _) | ValDecl(_, _, _, _, _, _) | AliasLine(_, _, _) => 99
    tasks(0) := Visit(decl)
    val Walker(_, _, _, _, walked, references, unresolved, _, fault) = Stigi.Names.walkFrom(Walker(tasks, 1, locals, 0, Edges(sources, targets, 0, stamps), 0, 0, 0, WalkSound), gamma, Members(memberPool, 0, table, 0, memberSlots), tables, 0)
    val Edges(edgeSources, edgeTargets, count, _) = walked
    val sound: Bool = fault is WalkSound
    if copied == 14 then
      if count == 2 then
        if edgeSources(0) == 0 && edgeSources(1) == 0 && edgeTargets(0) == 1 && edgeTargets(1) == 2 then
          if references == 3 then
            if unresolved == 0 then
              if sound then 0 else 6
            else 5
          else 4
        else 3
      else 2
    else 1
  if code == 0 then Stigi.Test.expect(true, true, "a pre-group fact resolves the parameter it names") else if code == 1 then Stigi.Test.expect(false, true, "clause copied names") else if code == 2 then Stigi.Test.expect(false, true, "clause edge count is two") else if code == 3 then Stigi.Test.expect(false, true, "clause edge endpoints") else if code == 4 then Stigi.Test.expect(false, true, "clause references count is three") else if code == 5 then Stigi.Test.expect(false, true, "clause unresolved count is zero") else if code == 6 then Stigi.Test.expect(false, true, "clause walk ends WalkSound") else Stigi.Test.expect(false, true, "clause setup allocates the tables, adds the three entries and parses the parameter sample")
