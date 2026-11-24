// prompt: iteratively lower a parsed Stigi syntax unit body into flat postorder Checked-IR tables with comparison swap and pipe desugaring.
def Stigi.Elaborate.lowerUnit {Stigi.Mut} (tables: Stigi.Syntax.Tables, rootSyn: Stigi.Syntax.NodeAt, nodeCap: U64, exprs: Array[Stigi.Core.Expr](nodeCap), listCap: U64, childLists: Array[U64](listCap), armCap: U64, arms: Array[Stigi.Core.Arm](armCap), workCap: U64, workNode: Array[U64](workCap), workPhase: Array[U8](workCap), mapCap: U64, mapped: Array[U64](mapCap), patRes: Array[Stigi.Elaborate.PatResolution](mapCap)) <below(0, nodeCap), below(0, listCap), below(0, armCap), below(0, workCap), lessOrEqual(1, workCap), below(0, mapCap)>: Stigi.Elaborate.LowerOutcome =
  val Tables(_, _, synNodes, _, _, _) = tables
  val rootBodyOpt: U64 = synNodes(rootSyn) match
    case UnitRoot(_, _, _, declIdx) =>
      synNodes(declIdx) match
        case FunctionDecl(_, _, _, _, _, _, bodyIdx) => bodyIdx
        case ValDecl(_, _, _, _, _, bodyIdx) => bodyIdx
        case Absent(_) | Name(_, _) | TypeRef(_, _, _, _, _) | FactAtom(_, _, _, _, _) | FactCompare(_, _, _, _, _, _) | FactArith(_, _, _, _, _, _, _) | FactDeny(_, _) | FactGroup(_, _) | Param(_, _, _) | CapParam(_, _, _) | CapGroup(_, _) | PatName(_, _) | PatLiteral(_, _) | PatCtor(_, _, _) | PatTuple(_, _) | PatOr(_, _) | PatRange(_, _, _) | Literal(_, _, _) | Ref(_, _) | Call(_, _, _, _, _, _) | Binary(_, _, _, _) | Unary(_, _, _) | IsTest(_, _, _) | IfElse(_, _, _, _) | WhileLoop(_, _, _) | MatchOn(_, _, _) | Arm(_, _, _, _) | ValStmt(_, _, _, _, _) | VarStmt(_, _, _, _, _) | AssignStmt(_, _, _) | IndexRead(_, _, _) | IndexWrite(_, _, _, _) | HandleStmt(_, _, _, _, _, _, _, _) | AssertStmt(_, _, _) | DenyStmt(_, _) | Block(_, _) | LocalDecl(_, _) | DefHead(_, _, _, _, _, _, _, _) | EnumDecl(_, _, _, _, _) | EnumCase(_, _, _, _, _) | TypeDecl(_, _, _, _, _, _, _, _) | CapabilityDecl(_, _, _, _, _, _, _, _) | HandlerDecl(_, _, _, _, _, _, _, _, _, _) | TraitDecl(_, _, _, _, _) | GivenDecl(_, _, _, _, _, _, _) | ExternDef(_, _, _, _, _, _) | ExternVal(_, _, _, _, _) | RelationDecl(_, _, _, _) | RuleDecl(_, _, _, _, _, _) | UnitRoot(_, _, _, _) | AliasLine(_, _, _) => rootSyn
    case AliasLine(_, _, _) | Absent(_) | Name(_, _) | TypeRef(_, _, _, _, _) | FactAtom(_, _, _, _, _) | FactCompare(_, _, _, _, _, _) | FactArith(_, _, _, _, _, _, _) | FactDeny(_, _) | FactGroup(_, _) | Param(_, _, _) | CapParam(_, _, _) | CapGroup(_, _) | PatName(_, _) | PatLiteral(_, _) | PatCtor(_, _, _) | PatTuple(_, _) | PatOr(_, _) | PatRange(_, _, _) | Literal(_, _, _) | Ref(_, _) | Call(_, _, _, _, _, _) | Binary(_, _, _, _) | Unary(_, _, _) | IsTest(_, _, _) | IfElse(_, _, _, _) | WhileLoop(_, _, _) | MatchOn(_, _, _) | Arm(_, _, _, _) | ValStmt(_, _, _, _, _) | VarStmt(_, _, _, _, _) | AssignStmt(_, _, _) | IndexRead(_, _, _) | IndexWrite(_, _, _, _) | HandleStmt(_, _, _, _, _, _, _, _) | AssertStmt(_, _, _) | DenyStmt(_, _) | Block(_, _) | LocalDecl(_, _) | DefHead(_, _, _, _, _, _, _, _) | FunctionDecl(_, _, _, _, _, _, _) | ValDecl(_, _, _, _, _, _) | EnumDecl(_, _, _, _, _) | EnumCase(_, _, _, _, _) | TypeDecl(_, _, _, _, _, _, _, _) | CapabilityDecl(_, _, _, _, _, _, _, _) | HandlerDecl(_, _, _, _, _, _, _, _, _, _) | TraitDecl(_, _, _, _, _) | GivenDecl(_, _, _, _, _, _, _) | ExternDef(_, _, _, _, _, _) | ExternVal(_, _, _, _, _) | RelationDecl(_, _, _, _) | RuleDecl(_, _, _, _, _, _) => rootSyn
  var topIdx: U64 = 1
  workNode(0) := rootBodyOpt
  workPhase(0) := U8.ofU64(0)
  var curCore: U64 = 0
  var curList: U64 = 0
  var curArm: U64 = 0
  var failed: Bool = false
  var overflowFlag: Bool = false
  while topIdx > 0 && !failed && !overflowFlag do
    @<lessOrEqual(topIdx, workCap), lessOrEqual(curCore, nodeCap), lessOrEqual(curList, listCap), lessOrEqual(curArm, armCap)>
    val popIdx: U64 = topIdx - 1
    topIdx = popIdx
    val synIdx: U64 = workNode(popIdx)
    val phaseVal: U64 = U64.ofU8(workPhase(popIdx))
    if synIdx >= Stigi.Limit.maxUnitNodes || synIdx >= mapCap then
      overflowFlag = true
    else if phaseVal == 0 then
      Stigi.Elaborate.pushChildren(tables, synIdx, workCap, workNode, workPhase, topIdx) match
        case PushFull => overflowFlag = true
        case PushDone(nextTop) =>
          if nextTop <= workCap then topIdx = nextTop else overflowFlag = true
    else if curCore >= nodeCap then
      overflowFlag = true
    else
      Stigi.Elaborate.emitCoreNode(tables, synIdx, nodeCap, exprs, curCore, listCap, childLists, curList, armCap, arms, curArm, mapCap, mapped, patRes) match
        case EmitFull => overflowFlag = true
        case EmitInvalid => failed = true
        case EmitDone(nextCore, nextList, nextArm) =>
          if nextCore <= nodeCap && nextList <= listCap && nextArm <= armCap then
            curCore = nextCore
            curList = nextList
            curArm = nextArm
          else
            overflowFlag = true
  if overflowFlag then LowerOverflow else if failed || rootBodyOpt >= mapCap then LowerInvalid else Lowered(mapped(rootBodyOpt), curCore, curArm, curList)
