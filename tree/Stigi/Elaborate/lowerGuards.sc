// prompt: lower the guarded arms of one match back to front into tail selects and choice nodes, then report where the full select goes.
def Stigi.Elaborate.lowerGuards {Stigi.Mut} (tables: Stigi.Syntax.Tables, armsSpan: Stigi.Syntax.Span, scrutCore: U64, emitIdx: U64, nodeCap: U64, exprs: Array[Stigi.Core.Expr](nodeCap), armCap: U64, arms: Array[Stigi.Core.Arm](armCap), curArm: U64, mapCap: U64, mapped: Array[U64](mapCap), patRes: Array[Stigi.Elaborate.PatResolution](mapCap)) <lessOrEqual(emitIdx, nodeCap), lessOrEqual(curArm, armCap)>: Stigi.Elaborate.EmitResult =
  val Tables(_, _, synNodes, synLists, _, _) = tables
  armsSpan match
    case Span(first, endPos) =>
      var pos: U64 = endPos
      var coreAt: U64 = emitIdx
      var armPos: U64 = curArm
      var outcome: Stigi.Elaborate.EmitResult = EmitDone(0, 0, curArm)
      while first < pos && 1 <= pos && outcome is EmitDone(_, _, _) do
        @<lessOrEqual(pos, endPos), lessOrEqual(armPos, armCap), lessOrEqual(coreAt, nodeCap)>
        val prevPos: U64 = pos - 1
        val armSyn: Stigi.Syntax.NodeAt = synLists(prevPos)
        synNodes(armSyn) match
          case Arm(_, _, guardSyn, bodySyn) =>
            if bodySyn < mapCap && guardSyn < mapCap && armSyn < mapCap then
              val bodyCore: U64 = mapped(bodySyn)
              if synNodes(guardSyn) is Absent(_) then
                mapped(armSyn) := bodyCore
              else
                val guardCore: U64 = mapped(guardSyn)
                if coreAt < nodeCap then
                  val choiceAt: U64 = coreAt + 1
                  if pos < endPos then
                    if choiceAt < nodeCap then
                      if scrutCore < coreAt && guardCore < coreAt && bodyCore < coreAt then
                        Stigi.Elaborate.lowerArms(tables, Span(pos, endPos), coreAt, armCap, arms, armPos, mapCap, mapped, patRes) match
                          case EmitFull => outcome = EmitFull
                          case EmitInvalid => outcome = EmitInvalid
                          case EmitDone(_, _, nextArm) =>
                            if armPos <= nextArm && nextArm <= armCap then
                              exprs(coreAt) := SelectExpr(coreAt, scrutCore, CoreSpan(armPos, nextArm), 0)
                              exprs(choiceAt) := ChoiceExpr(choiceAt, guardCore, bodyCore, coreAt, 0)
                              mapped(armSyn) := choiceAt
                              armPos = nextArm
                              coreAt = choiceAt + 1
                            else outcome = EmitFull
                      else
                        outcome = EmitInvalid
                    else
                      outcome = EmitFull
                  else
                    outcome = EmitInvalid
                else
                  outcome = EmitFull
            else
              outcome = EmitInvalid
          case Absent(_) | Name(_, _) | TypeRef(_, _, _, _, _) | FactAtom(_, _, _, _, _) | FactCompare(_, _, _, _, _, _) | FactArith(_, _, _, _, _, _, _) | FactDeny(_, _) | FactGroup(_, _) | Param(_, _, _) | CapParam(_, _, _) | CapGroup(_, _) | PatName(_, _) | PatLiteral(_, _) | PatCtor(_, _, _) | PatTuple(_, _) | PatOr(_, _) | PatRange(_, _, _) | Literal(_, _, _) | Ref(_, _) | Call(_, _, _, _, _, _) | Binary(_, _, _, _) | Unary(_, _, _) | IsTest(_, _, _) | IfElse(_, _, _, _) | WhileLoop(_, _, _) | MatchOn(_, _, _) | ValStmt(_, _, _, _, _) | VarStmt(_, _, _, _, _) | AssignStmt(_, _, _) | IndexRead(_, _, _) | IndexWrite(_, _, _, _) | HandleStmt(_, _, _, _, _, _, _, _) | AssertStmt(_, _, _) | DenyStmt(_, _) | Block(_, _) | LocalDecl(_, _) | DefHead(_, _, _, _, _, _, _, _) | FunctionDecl(_, _, _, _, _, _, _) | ValDecl(_, _, _, _, _, _) | EnumDecl(_, _, _, _, _) | EnumCase(_, _, _, _, _) | TypeDecl(_, _, _, _, _, _, _, _) | CapabilityDecl(_, _, _, _, _, _, _, _) | HandlerDecl(_, _, _, _, _, _, _, _, _, _) | TraitDecl(_, _, _, _, _) | GivenDecl(_, _, _, _, _, _, _) | ExternDef(_, _, _, _, _, _) | ExternVal(_, _, _, _, _) | RelationDecl(_, _, _, _) | RuleDecl(_, _, _, _, _, _) | UnitRoot(_, _, _, _) | AliasLine(_, _, _) =>
            outcome = EmitInvalid
        pos = prevPos
      outcome match
        case EmitFull => EmitFull
        case EmitInvalid => EmitInvalid
        case EmitDone(_, _, _) =>
          Stigi.Elaborate.lowerArms(tables, armsSpan, coreAt, armCap, arms, armPos, mapCap, mapped, patRes) match
            case EmitFull => EmitFull
            case EmitInvalid => EmitInvalid
            case EmitDone(_, _, endArm) => EmitDone(coreAt, armPos, endArm)
