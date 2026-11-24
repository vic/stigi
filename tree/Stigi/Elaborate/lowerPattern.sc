// prompt: lower the pattern of one unguarded match arm, one decision arm per or-alternative, all sharing the arm body.
def Stigi.Elaborate.lowerPattern {Stigi.Mut} (tables: Stigi.Syntax.Tables, patSyn: Stigi.Syntax.NodeAt, bodyAt: U64, armCap: U64, arms: Array[Stigi.Core.Arm](armCap), curArm: U64, mapCap: U64, patRes: Array[Stigi.Elaborate.PatResolution](mapCap)) <below(patSyn, mapCap), lessOrEqual(curArm, armCap)>: Stigi.Elaborate.EmitResult =
  val Tables(_, _, synNodes, synLists, _, _) = tables
  synNodes(patSyn) match
    case PatOr(_, alternatives) =>
      alternatives match
        case Span(altFirst, altEnd) =>
          var pos: U64 = altFirst
          var armPos: U64 = curArm
          var outcome: Stigi.Elaborate.EmitResult = EmitDone(0, 0, curArm)
          while pos < altEnd && outcome is EmitDone(_, _, _) do
            @<lessOrEqual(altFirst, pos), lessOrEqual(pos, altEnd), lessOrEqual(armPos, armCap)>
            val nextPos: U64 = pos + 1
            val altSyn: Stigi.Syntax.NodeAt = synLists(pos)
            if altSyn < mapCap then
              Stigi.Elaborate.lowerAlt(tables, altSyn, bodyAt, armCap, arms, armPos, mapCap, patRes) match
                case EmitFull => outcome = EmitFull
                case EmitInvalid => outcome = EmitInvalid
                case EmitDone(_, _, nextArm) =>
                  if nextArm <= armCap then armPos = nextArm else outcome = EmitFull
            else
              outcome = EmitInvalid
            pos = nextPos
          outcome match
            case EmitDone(_, _, _) => EmitDone(0, 0, armPos)
            case EmitFull => EmitFull
            case EmitInvalid => EmitInvalid
    case Absent(_) | Name(_, _) | TypeRef(_, _, _, _, _) | FactAtom(_, _, _, _, _) | FactCompare(_, _, _, _, _, _) | FactArith(_, _, _, _, _, _, _) | FactDeny(_, _) | FactGroup(_, _) | Param(_, _, _) | CapParam(_, _, _) | CapGroup(_, _) | PatName(_, _) | PatLiteral(_, _) | PatCtor(_, _, _) | PatTuple(_, _) | PatRange(_, _, _) | Literal(_, _, _) | Ref(_, _) | Call(_, _, _, _, _, _) | Binary(_, _, _, _) | Unary(_, _, _) | IsTest(_, _, _) | IfElse(_, _, _, _) | WhileLoop(_, _, _) | MatchOn(_, _, _) | Arm(_, _, _, _) | ValStmt(_, _, _, _, _) | VarStmt(_, _, _, _, _) | AssignStmt(_, _, _) | IndexRead(_, _, _) | IndexWrite(_, _, _, _) | HandleStmt(_, _, _, _, _, _, _, _) | AssertStmt(_, _, _) | DenyStmt(_, _) | Block(_, _) | LocalDecl(_, _) | DefHead(_, _, _, _, _, _, _, _) | FunctionDecl(_, _, _, _, _, _, _) | ValDecl(_, _, _, _, _, _) | EnumDecl(_, _, _, _, _) | EnumCase(_, _, _, _, _) | TypeDecl(_, _, _, _, _, _, _, _) | CapabilityDecl(_, _, _, _, _, _, _, _) | HandlerDecl(_, _, _, _, _, _, _, _, _, _) | TraitDecl(_, _, _, _, _) | GivenDecl(_, _, _, _, _, _, _) | ExternDef(_, _, _, _, _, _) | ExternVal(_, _, _, _, _) | RelationDecl(_, _, _, _) | RuleDecl(_, _, _, _, _, _) | UnitRoot(_, _, _, _) | AliasLine(_, _, _) =>
      Stigi.Elaborate.lowerAlt(tables, patSyn, bodyAt, armCap, arms, curArm, mapCap, patRes)
