// prompt: lower one pattern alternative of an unguarded match arm into one decision arm that targets the shared arm body.
def Stigi.Elaborate.lowerAlt {Stigi.Mut} (tables: Stigi.Syntax.Tables, patSyn: U64, bodyAt: U64, core: Stigi.Core.Tables, scratch: Stigi.Elaborate.Scratch) <below(patSyn, Stigi.Limit.maxUnitNodes)>: Stigi.Elaborate.CoreResult =
  val Tables(_, _, synNodes, _, _, _) = tables
  scratch match
    case WorkScratch(capacity, _, _, _, patRes) =>
      core match
        case CoreTables(expressionCapacity, exprs, expressionMark, exprEnd, listCapacity, childLists, listMark, listEnd, armCapacity, arms, armMark, curArm) =>
          if patSyn < capacity then
            if curArm < armCapacity then
              val nextArm: U64 = curArm + 1
              synNodes(patSyn) match
                case PatCtor(_, _, parts) =>
                  parts match
                    case Span(partFirst, partEnd) =>
                      if partFirst < partEnd then CoreInvalid else
                        patRes(patSyn) match
                          case PatCaseOrd(caseIdx) =>
                            arms(curArm) := CaseArm(caseIdx, CoreSpan(0, 0), bodyAt)
                            CoreDone(CoreTables(expressionCapacity, exprs, expressionMark, exprEnd, listCapacity, childLists, listMark, listEnd, armCapacity, arms, armMark, nextArm))
                          case PatLocalBind => CoreInvalid
                case PatName(_, _) =>
                  patRes(patSyn) match
                    case PatCaseOrd(caseIdx) =>
                      arms(curArm) := CaseArm(caseIdx, CoreSpan(0, 0), bodyAt)
                      CoreDone(CoreTables(expressionCapacity, exprs, expressionMark, exprEnd, listCapacity, childLists, listMark, listEnd, armCapacity, arms, armMark, nextArm))
                    case PatLocalBind =>
                      arms(curArm) := RestArm(bodyAt)
                      CoreDone(CoreTables(expressionCapacity, exprs, expressionMark, exprEnd, listCapacity, childLists, listMark, listEnd, armCapacity, arms, armMark, nextArm))
                case PatLiteral(_, _) =>
                  Stigi.Elaborate.patBound(tables, patSyn, capacity) match
                    case BadIntLiteral => CoreInvalid
                    case ParsedInt(only) =>
                      arms(curArm) := ValueArm(only, only, bodyAt)
                      CoreDone(CoreTables(expressionCapacity, exprs, expressionMark, exprEnd, listCapacity, childLists, listMark, listEnd, armCapacity, arms, armMark, nextArm))
                case PatRange(_, lowSyn, highSyn) =>
                  if lowSyn < capacity && highSyn < capacity then
                    Stigi.Elaborate.patBound(tables, lowSyn, capacity) match
                      case BadIntLiteral => CoreInvalid
                      case ParsedInt(lowVal) =>
                        Stigi.Elaborate.patBound(tables, highSyn, capacity) match
                          case BadIntLiteral => CoreInvalid
                          case ParsedInt(highVal) =>
                            if lowVal <= highVal then
                              arms(curArm) := ValueArm(lowVal, highVal, bodyAt)
                              CoreDone(CoreTables(expressionCapacity, exprs, expressionMark, exprEnd, listCapacity, childLists, listMark, listEnd, armCapacity, arms, armMark, nextArm))
                            else CoreInvalid
                  else CoreFull
                case Absent(_) | Name(_, _) | TypeRef(_, _, _, _, _) | FactAtom(_, _, _, _, _) | FactCompare(_, _, _, _, _, _) | FactArith(_, _, _, _, _, _, _) | FactDeny(_, _) | FactGroup(_, _) | Param(_, _, _) | CapParam(_, _, _) | CapGroup(_, _) | PatTuple(_, _) | PatOr(_, _) | Literal(_, _, _) | Ref(_, _) | Call(_, _, _, _, _, _) | Binary(_, _, _, _) | Unary(_, _, _) | IsTest(_, _, _) | IfElse(_, _, _, _) | WhileLoop(_, _, _) | MatchOn(_, _, _) | Arm(_, _, _, _) | ValStmt(_, _, _, _, _) | VarStmt(_, _, _, _, _) | AssignStmt(_, _, _) | IndexRead(_, _, _) | IndexWrite(_, _, _, _) | HandleStmt(_, _, _, _, _, _, _, _) | AssertStmt(_, _, _) | DenyStmt(_, _) | Block(_, _) | LocalDecl(_, _) | DefHead(_, _, _, _, _, _, _, _) | FunctionDecl(_, _, _, _, _, _, _) | ValDecl(_, _, _, _, _, _) | EnumDecl(_, _, _, _, _) | EnumCase(_, _, _, _, _) | TypeDecl(_, _, _, _, _, _, _, _) | CapabilityDecl(_, _, _, _, _, _, _, _) | HandlerDecl(_, _, _, _, _, _, _, _, _, _) | TraitDecl(_, _, _, _, _) | GivenDecl(_, _, _, _, _, _, _) | ExternDef(_, _, _, _, _, _) | ExternVal(_, _, _, _, _) | RelationDecl(_, _, _, _) | RuleDecl(_, _, _, _, _, _) | UnitRoot(_, _, _, _) | AliasLine(_, _, _) => CoreInvalid
            else CoreFull
          else CoreFull
