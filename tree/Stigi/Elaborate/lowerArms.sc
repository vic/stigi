// prompt: lower the arms of one match span into one-level decision arms whose bodies, guarded or not, are already emitted below the select.
def Stigi.Elaborate.lowerArms {Stigi.Mut} (tables: Stigi.Syntax.Tables, armsSpan: Stigi.Syntax.Span, selectIdx: U64, armCap: U64, arms: Array[Stigi.Core.Arm](armCap), curArm: U64, mapCap: U64, mapped: Array[U64](mapCap), patRes: Array[Stigi.Elaborate.PatResolution](mapCap)) <lessOrEqual(curArm, armCap)>: Stigi.Elaborate.EmitResult =
  val Tables(_, _, synNodes, synLists, _, _) = tables
  armsSpan match
    case Span(first, endPos) =>
      var pos: U64 = first
      var armPos: U64 = curArm
      var outcome: Stigi.Elaborate.EmitResult = EmitDone(0, 0, curArm)
      while pos < endPos && outcome is EmitDone(_, _, _) do
        @<lessOrEqual(first, pos), lessOrEqual(pos, endPos), lessOrEqual(armPos, armCap)>
        val nextPos: U64 = pos + 1
        val armSyn: Stigi.Syntax.NodeAt = synLists(pos)
        synNodes(armSyn) match
          case Arm(_, patSyn, _, _) =>
            if patSyn < mapCap && armSyn < mapCap then
              val bodyAt: U64 = mapped(armSyn)
              if bodyAt < selectIdx then
                Stigi.Elaborate.lowerPattern(tables, patSyn, bodyAt, armCap, arms, armPos, mapCap, patRes) match
                  case EmitFull => outcome = EmitFull
                  case EmitInvalid => outcome = EmitInvalid
                  case EmitDone(_, _, nextArm) =>
                    if nextArm <= armCap then armPos = nextArm else outcome = EmitFull
              else
                outcome = EmitInvalid
            else
              outcome = EmitInvalid
          case Absent(_) | Name(_, _) | TypeRef(_, _, _, _, _) | FactAtom(_, _, _, _, _) | FactCompare(_, _, _, _, _, _) | FactArith(_, _, _, _, _, _, _) | FactDeny(_, _) | FactGroup(_, _) | Param(_, _, _) | CapParam(_, _, _) | CapGroup(_, _) | PatName(_, _) | PatLiteral(_, _) | PatCtor(_, _, _) | PatTuple(_, _) | PatOr(_, _) | PatRange(_, _, _) | Literal(_, _, _) | Ref(_, _) | Call(_, _, _, _, _, _) | Binary(_, _, _, _) | Unary(_, _, _) | IsTest(_, _, _) | IfElse(_, _, _, _) | WhileLoop(_, _, _) | MatchOn(_, _, _) | ValStmt(_, _, _, _, _) | VarStmt(_, _, _, _, _) | AssignStmt(_, _, _) | IndexRead(_, _, _) | IndexWrite(_, _, _, _) | HandleStmt(_, _, _, _, _, _, _, _) | AssertStmt(_, _, _) | DenyStmt(_, _) | Block(_, _) | LocalDecl(_, _) | DefHead(_, _, _, _, _, _, _, _) | FunctionDecl(_, _, _, _, _, _, _) | ValDecl(_, _, _, _, _, _) | EnumDecl(_, _, _, _, _) | EnumCase(_, _, _, _, _) | TypeDecl(_, _, _, _, _, _, _, _) | CapabilityDecl(_, _, _, _, _, _, _, _) | HandlerDecl(_, _, _, _, _, _, _, _, _, _) | TraitDecl(_, _, _, _, _) | GivenDecl(_, _, _, _, _, _, _) | ExternDef(_, _, _, _, _, _) | ExternVal(_, _, _, _, _) | RelationDecl(_, _, _, _) | RuleDecl(_, _, _, _, _, _) | UnitRoot(_, _, _, _) | AliasLine(_, _, _) =>
            outcome = EmitInvalid
        pos = nextPos
      outcome match
        case EmitDone(_, _, _) => EmitDone(0, 0, armPos)
        case EmitFull => EmitFull
        case EmitInvalid => EmitInvalid
