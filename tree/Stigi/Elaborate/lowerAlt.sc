// prompt: lower one pattern alternative of an unguarded match arm into one decision arm that targets the shared arm body.
def Stigi.Elaborate.lowerAlt {Stigi.Mut} (tables: Stigi.Syntax.Tables, patSyn: Stigi.Syntax.NodeAt, bodyAt: U64, armCap: U64, arms: Array[Stigi.Core.Arm](armCap), curArm: U64, mapCap: U64, patRes: Array[Stigi.Elaborate.PatResolution](mapCap)) <below(patSyn, mapCap), lessOrEqual(curArm, armCap)>: Stigi.Elaborate.EmitResult =
  val Tables(_, _, synNodes, _, _, _) = tables
  if curArm < armCap then
    val nextArm: U64 = curArm + 1
    synNodes(patSyn) match
      case PatCtor(_, _, parts) =>
        parts match
          case Span(partFirst, partEnd) =>
            if partFirst < partEnd then EmitInvalid else
              patRes(patSyn) match
                case PatCaseOrd(caseIdx) =>
                  arms(curArm) := CaseArm(caseIdx, CoreSpan(0, 0), bodyAt)
                  EmitDone(0, 0, nextArm)
                case PatLocalBind => EmitInvalid
      case PatName(_, _) =>
        patRes(patSyn) match
          case PatCaseOrd(caseIdx) =>
            arms(curArm) := CaseArm(caseIdx, CoreSpan(0, 0), bodyAt)
            EmitDone(0, 0, nextArm)
          case PatLocalBind =>
            arms(curArm) := RestArm(bodyAt)
            EmitDone(0, 0, nextArm)
      case PatLiteral(_, _) =>
        Stigi.Elaborate.patBound(tables, patSyn, mapCap) match
          case BadIntLiteral => EmitInvalid
          case ParsedInt(only) =>
            arms(curArm) := ValueArm(only, only, bodyAt)
            EmitDone(0, 0, nextArm)
      case PatRange(_, lowSyn, highSyn) =>
        if lowSyn < mapCap && highSyn < mapCap then
          Stigi.Elaborate.patBound(tables, lowSyn, mapCap) match
            case BadIntLiteral => EmitInvalid
            case ParsedInt(lowVal) =>
              Stigi.Elaborate.patBound(tables, highSyn, mapCap) match
                case BadIntLiteral => EmitInvalid
                case ParsedInt(highVal) =>
                  if lowVal <= highVal then
                    arms(curArm) := ValueArm(lowVal, highVal, bodyAt)
                    EmitDone(0, 0, nextArm)
                  else EmitInvalid
        else EmitInvalid
      case Absent(_) | Name(_, _) | TypeRef(_, _, _, _, _) | FactAtom(_, _, _, _, _) | FactCompare(_, _, _, _, _, _) | FactArith(_, _, _, _, _, _, _) | FactDeny(_, _) | FactGroup(_, _) | Param(_, _, _) | CapParam(_, _, _) | CapGroup(_, _) | PatTuple(_, _) | PatOr(_, _) | Literal(_, _, _) | Ref(_, _) | Call(_, _, _, _, _, _) | Binary(_, _, _, _) | Unary(_, _, _) | IsTest(_, _, _) | IfElse(_, _, _, _) | WhileLoop(_, _, _) | MatchOn(_, _, _) | Arm(_, _, _, _) | ValStmt(_, _, _, _, _) | VarStmt(_, _, _, _, _) | AssignStmt(_, _, _) | IndexRead(_, _, _) | IndexWrite(_, _, _, _) | HandleStmt(_, _, _, _, _, _, _, _) | AssertStmt(_, _, _) | DenyStmt(_, _) | Block(_, _) | LocalDecl(_, _) | DefHead(_, _, _, _, _, _, _, _) | FunctionDecl(_, _, _, _, _, _, _) | ValDecl(_, _, _, _, _, _) | EnumDecl(_, _, _, _, _) | EnumCase(_, _, _, _, _) | TypeDecl(_, _, _, _, _, _, _, _) | CapabilityDecl(_, _, _, _, _, _, _, _) | HandlerDecl(_, _, _, _, _, _, _, _, _, _) | TraitDecl(_, _, _, _, _) | GivenDecl(_, _, _, _, _, _, _) | ExternDef(_, _, _, _, _, _) | ExternVal(_, _, _, _, _) | RelationDecl(_, _, _, _) | RuleDecl(_, _, _, _, _, _) | UnitRoot(_, _, _, _) | AliasLine(_, _, _) => EmitInvalid
  else EmitFull
