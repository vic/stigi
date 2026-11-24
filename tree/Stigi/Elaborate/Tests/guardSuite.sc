// prompt: verify real parsed guarded arm lowering into a choice whose fallback is the shared tail select of the following arms.
val Stigi.Elaborate.Tests.guardSuite {Stigi.Test}: Stigi.Test.Result =
  val srcGuard: Array[U8](128) = "// prompt: pick demo.\ndef Demo.pick(x: Demo.Pick): U64 =\n  x match\n    case One if x => 1\n    case Two => 2\n    case other => 3\n"
  val zeroWord: U64 = 0
  val zeroByte: U8 = U8.ofU64(0)
  val initExpr: Stigi.Core.Expr = NothingExpr(0)
  val initArm: Stigi.Core.Arm = RestArm(0)
  val initRes: Stigi.Elaborate.PatResolution = PatLocalBind
  val allPass: Bool = Stigi.Syntax.allocate{Stigi.Region}() match
    case AllocationFailed => false
    case Allocated(tables) =>
      Array.tryFill{Stigi.Region}(16, initExpr) match
        case FillFailed => false
        case Filled(exprs) =>
          Array.tryFill{Stigi.Region}(8, initArm) match
            case FillFailed => false
            case Filled(arms) =>
              Array.tryFill{Stigi.Region}(8, zeroWord) match
                case FillFailed => false
                case Filled(childLists) =>
                  Array.tryFill{Stigi.Region}(64, zeroWord) match
                    case FillFailed => false
                    case Filled(workNode) =>
                      Array.tryFill{Stigi.Region}(64, zeroByte) match
                        case FillFailed => false
                        case Filled(workPhase) =>
                          Array.tryFill{Stigi.Region}(128, zeroWord) match
                            case FillFailed => false
                            case Filled(mapped) =>
                              Array.tryFill{Stigi.Region}(128, initRes) match
                                case FillFailed => false
                                case Filled(patRes) =>
                                  Stigi.Syntax.Parse.parseUnit(tables, 128, srcGuard, 128) match
                                    case ParseRejected(_, _) => false
                                    case LexRejected(_) => false
                                    case Parsed(rootGuard, _, _) =>
                                      val Tables(source, _, synNodes, _, _, _) = tables
                                      var idx: U64 = 0
                                      var nextOrd: U64 = 2
                                      while idx < 64 do
                                        @<lessOrEqual(idx, 64), lessOrEqual(nextOrd, 8)>
                                        val nextIdx: U64 = idx + 1
                                        val nameText: Stigi.Syntax.Text = synNodes(idx) match
                                          case PatName(_, patText) => patText
                                          case PatCtor(_, ctorText, _) => ctorText
                                          case Absent(_) | Name(_, _) | TypeRef(_, _, _, _, _) | FactAtom(_, _, _, _, _) | FactCompare(_, _, _, _, _, _) | FactArith(_, _, _, _, _, _, _) | FactDeny(_, _) | FactGroup(_, _) | Param(_, _, _) | CapParam(_, _, _) | CapGroup(_, _) | PatLiteral(_, _) | PatTuple(_, _) | PatOr(_, _) | PatRange(_, _, _) | Literal(_, _, _) | Ref(_, _) | Call(_, _, _, _, _, _) | Binary(_, _, _, _) | Unary(_, _, _) | IsTest(_, _, _) | IfElse(_, _, _, _) | WhileLoop(_, _, _) | MatchOn(_, _, _) | Arm(_, _, _, _) | ValStmt(_, _, _, _, _) | VarStmt(_, _, _, _, _) | AssignStmt(_, _, _) | IndexRead(_, _, _) | IndexWrite(_, _, _, _) | HandleStmt(_, _, _, _, _, _, _, _) | AssertStmt(_, _, _) | DenyStmt(_, _) | Block(_, _) | LocalDecl(_, _) | DefHead(_, _, _, _, _, _, _, _) | FunctionDecl(_, _, _, _, _, _, _) | ValDecl(_, _, _, _, _, _) | EnumDecl(_, _, _, _, _) | EnumCase(_, _, _, _, _) | TypeDecl(_, _, _, _, _, _, _, _) | CapabilityDecl(_, _, _, _, _, _, _, _) | HandlerDecl(_, _, _, _, _, _, _, _, _, _) | TraitDecl(_, _, _, _, _) | GivenDecl(_, _, _, _, _, _, _) | ExternDef(_, _, _, _, _, _) | ExternVal(_, _, _, _, _) | RelationDecl(_, _, _, _) | RuleDecl(_, _, _, _, _, _) | UnitRoot(_, _, _, _) | AliasLine(_, _, _) => Text(0, 0)
                                        val upper: Bool = nameText match
                                          case Text(nameFirst, nameEnd) =>
                                            if nameFirst < nameEnd && nameFirst < 128 then
                                              val firstByte: U64 = U64.ofU8(source(nameFirst))
                                              firstByte >= 65 && firstByte <= 90
                                            else false
                                        if upper && nextOrd < 7 then
                                          patRes(idx) := PatCaseOrd(nextOrd)
                                          nextOrd = nextOrd + 1
                                        else
                                          patRes(idx) := PatLocalBind
                                        idx = nextIdx
                                      Stigi.Elaborate.lowerUnit(tables, rootGuard, 16, exprs, 8, childLists, 8, arms, 64, workNode, workPhase, 128, mapped, patRes) match
                                        case LowerOverflow => false
                                        case LowerInvalid => false
                                        case Lowered(rootCore, exprCount, armCount, listCount) =>
                                          if exprCount <= 16 && armCount <= 8 && listCount <= 8 then
                                            val validGuard: Bool = Stigi.Core.validateExprTable(16, exprs, exprCount, 8, arms, armCount, 8, childLists, listCount) match
                                              case TableValid(cnt) => cnt == exprCount
                                              case MalformedExprTable(_) => false
                                            val shapeGuard: Bool = exprCount == 8 && armCount == 5 && rootCore == 7 && exprs(0) is LocalExpr(_, _, _) && exprs(7) is SelectExpr(_, 0, CoreSpan(2, 5), _) && exprs(5) is SelectExpr(_, 0, CoreSpan(0, 2), _)
                                            val choiceGuard: Bool = exprs(6) is ChoiceExpr(_, 3, 4, 5, _)
                                            val armsGuard: Bool = arms(0) is CaseArm(3, _, 2) && arms(1) is RestArm(1) && arms(2) is CaseArm(2, _, 6) && arms(3) is CaseArm(3, _, 2) && arms(4) is RestArm(1)
                                            validGuard && shapeGuard && choiceGuard && armsGuard
                                          else false
  Stigi.Test.expect(allPass, true, "Stigi.Elaborate must lower a guarded arm into a choice whose fallback is the tail select of the following arms")
