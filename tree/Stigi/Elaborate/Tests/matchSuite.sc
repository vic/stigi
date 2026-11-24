// prompt: verify real parsed single-scrutinee unguarded match lowering into one select with case arms, or-alternative arms sharing one body, and a rest arm.
val Stigi.Elaborate.Tests.matchSuite {Stigi.Test}: Stigi.Test.Result =
  val srcMatch: Array[U8](131) = "// prompt: pick demo.\ndef Demo.pick(x: Demo.Pick): U64 =\n  x match\n    case One => 1\n    case Two | Three => 2\n    case other => 3\n"
  val zeroWord: U64 = 0
  val initWork: Stigi.Elaborate.Work = Enter(0)
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
                  Array.tryFill{Stigi.Region}(128, initWork) match
                    case FillFailed => false
                    case Filled(work) =>
                      Array.tryFill{Stigi.Region}(128, zeroWord) match
                        case FillFailed => false
                        case Filled(mapped) =>
                          Array.tryFill{Stigi.Region}(128, initRes) match
                            case FillFailed => false
                            case Filled(patRes) =>
                              Stigi.Syntax.Parse.parseUnit(tables, 131, srcMatch, 131) match
                                case ParseRejected(_, _) => false
                                case LexRejected(_) => false
                                case Parsed(rootMatch, _, _) =>
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
                                        if nameFirst < nameEnd && nameFirst < 131 then
                                          val firstByte: U64 = U64.ofU8(source(nameFirst))
                                          firstByte >= 65 && firstByte <= 90
                                        else false
                                    if upper && nextOrd < 7 then
                                      patRes(idx) := PatCaseOrd(nextOrd)
                                      nextOrd = nextOrd + 1
                                    else
                                      patRes(idx) := PatLocalBind
                                    idx = nextIdx
                                  Stigi.Elaborate.lowerUnit(tables, rootMatch, CoreTables(16, exprs, 0, 0, 8, childLists, 0, 0, 8, arms, 0, 0), WorkScratch(128, work, 0, mapped, patRes)) match
                                    case LowerOverflow => false
                                    case LowerInvalid => false
                                    case Lowered(rootCore, CoreTables(_, _, _, exprCount, _, _, _, listCount, _, _, _, armCount), _) =>
                                      if exprCount <= 16 && armCount <= 8 && listCount <= 8 then
                                        val validMatch: Bool = Stigi.Core.validateExprTable(16, exprs, exprCount, 8, arms, armCount, 8, childLists, listCount) match
                                          case TableValid(cnt) => cnt == exprCount
                                          case MalformedExprTable(_) => false
                                        val shapeMatch: Bool = exprCount == 5 && armCount == 4 && rootCore == 4 && exprs(0) is LocalExpr(_, _, _) && exprs(4) is SelectExpr(_, 0, CoreSpan(0, 4), _)
                                        val armsMatch: Bool = arms(0) is CaseArm(2, _, 3) && arms(1) is CaseArm(3, _, 2) && arms(2) is CaseArm(4, _, 2) && arms(3) is RestArm(1)
                                        validMatch && shapeMatch && armsMatch
                                      else false
  Stigi.Test.expect(allPass, true, "Stigi.Elaborate must lower a match into one select with case arms, or-alternatives sharing one body, and a rest arm")
