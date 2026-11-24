// prompt: verify real parsed chained guarded arms lower into nested choices whose fallbacks are the tail selects of the following arms.
val Stigi.Elaborate.Tests.guardChainSuite {Stigi.Test}: Stigi.Test.Result =
  val srcChain: Array[U8](133) = "// prompt: pick demo.\ndef Demo.pick(x: Demo.Pick): U64 =\n  x match\n    case One if x => 1\n    case Two if x => 2\n    case other => 3\n"
  val zeroWord: U64 = 0
  val zeroByte: U8 = U8.ofU64(0)
  val initExpr: Stigi.Core.Expr = NothingExpr(0)
  val initArm: Stigi.Core.Arm = RestArm(0)
  val initRes: Stigi.Elaborate.PatResolution = PatLocalBind
  Stigi.Syntax.allocate{Stigi.Region}() match
    case AllocationFailed => Stigi.Test.expect(false, true, "the syntax tables must allocate")
    case Allocated(tables) =>
      Array.tryFill{Stigi.Region}(16, initExpr) match
        case FillFailed => Stigi.Test.expect(false, true, "the expression table must allocate")
        case Filled(exprs) =>
          Array.tryFill{Stigi.Region}(8, initArm) match
            case FillFailed => Stigi.Test.expect(false, true, "the arm table must allocate")
            case Filled(arms) =>
              Array.tryFill{Stigi.Region}(8, zeroWord) match
                case FillFailed => Stigi.Test.expect(false, true, "the list table must allocate")
                case Filled(childLists) =>
                  Array.tryFill{Stigi.Region}(64, zeroWord) match
                    case FillFailed => Stigi.Test.expect(false, true, "the work nodes must allocate")
                    case Filled(workNode) =>
                      Array.tryFill{Stigi.Region}(64, zeroByte) match
                        case FillFailed => Stigi.Test.expect(false, true, "the work phases must allocate")
                        case Filled(workPhase) =>
                          Array.tryFill{Stigi.Region}(128, zeroWord) match
                            case FillFailed => Stigi.Test.expect(false, true, "the node map must allocate")
                            case Filled(mapped) =>
                              Array.tryFill{Stigi.Region}(128, initRes) match
                                case FillFailed => Stigi.Test.expect(false, true, "the pattern table must allocate")
                                case Filled(patRes) =>
                                  Stigi.Syntax.Parse.parseUnit(tables, 133, srcChain, 133) match
                                    case ParseRejected(_, _) => Stigi.Test.expect(false, true, "the chained guard source must parse")
                                    case LexRejected(_) => Stigi.Test.expect(false, true, "the chained guard source must lex")
                                    case Parsed(rootChain, _, _) =>
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
                                      Stigi.Elaborate.lowerUnit(tables, rootChain, 16, exprs, 8, childLists, 8, arms, 64, workNode, workPhase, 128, mapped, patRes) match
                                        case LowerOverflow => Stigi.Test.expect(false, true, "the chained guards must fit the tables")
                                        case LowerInvalid => Stigi.Test.expect(false, true, "the chained guards must lower")
                                        case Lowered(rootCore, exprCount, armCount, listCount) =>
                                          if exprCount <= 16 && armCount <= 8 && listCount <= 8 then
                                            Stigi.Test.expect(exprCount, 11, "the chain must lower to eleven expressions")
                                            Stigi.Test.expect(armCount, 6, "the chain must lower to six decision arms")
                                            Stigi.Test.expect(rootCore, 10, "the root must be the full select")
                                            Stigi.Test.expect(exprs(6) is SelectExpr(_, 0, CoreSpan(0, 1), _), true, "the innermost tail select must hold only the catch-all arm")
                                            Stigi.Test.expect(exprs(7) is ChoiceExpr(_, 2, 3, 6, _), true, "the inner choice must fall back to the innermost tail select")
                                            Stigi.Test.expect(exprs(8) is SelectExpr(_, 0, CoreSpan(1, 3), _), true, "the outer tail select must hold the second arm and the catch-all")
                                            Stigi.Test.expect(arms(1) is CaseArm(3, _, 7), true, "the second arm of the outer tail select must reference the inner choice")
                                            Stigi.Test.expect(exprs(9) is ChoiceExpr(_, 4, 5, 8, _), true, "the outer choice must fall back to the outer tail select")
                                            Stigi.Test.expect(exprs(10) is SelectExpr(_, 0, CoreSpan(3, 6), _), true, "the full select must hold all three arms")
                                            Stigi.Test.expect(arms(3) is CaseArm(2, _, 9), true, "the first full arm must reference the outer choice")
                                            Stigi.Test.expect(arms(4) is CaseArm(3, _, 7), true, "the second full arm must reference the inner choice")
                                            Stigi.Test.expect(arms(5) is RestArm(1), true, "the last full arm must be the catch-all body")
                                            val validChain: Bool = Stigi.Core.validateExprTable(16, exprs, exprCount, 8, arms, armCount, 8, childLists, listCount) match
                                              case TableValid(cnt) => cnt == exprCount
                                              case MalformedExprTable(_) => false
                                            Stigi.Test.expect(validChain, true, "the chained table must validate")
                                          else
                                            Stigi.Test.expect(false, true, "the counts must stay within the capacities")
