// prompt: verify real parsed integer range and literal arms lower into value arms sharing their bodies.
val Stigi.Elaborate.Tests.sugarSuite {Stigi.Test}: Stigi.Test.Result =
  val srcSugar: Array[U8](116) = "// prompt: pick demo.\ndef Demo.pick(x: U64): U64 =\n  x match\n    case 1..3 => 1\n    case 7 => 2\n    case other => 3\n"
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
                                  Stigi.Syntax.Parse.parseUnit(tables, 116, srcSugar, 116) match
                                    case ParseRejected(_, _) => false
                                    case LexRejected(_) => false
                                    case Parsed(rootSugar, _, _) =>
                                      Stigi.Elaborate.lowerUnit(tables, rootSugar, 16, exprs, 8, childLists, 8, arms, 64, workNode, workPhase, 128, mapped, patRes) match
                                        case LowerOverflow => false
                                        case LowerInvalid => false
                                        case Lowered(rootCore, exprCount, armCount, listCount) =>
                                          if exprCount <= 16 && armCount <= 8 && listCount <= 8 then
                                            val validSugar: Bool = Stigi.Core.validateExprTable(16, exprs, exprCount, 8, arms, armCount, 8, childLists, listCount) match
                                              case TableValid(cnt) => cnt == exprCount
                                              case MalformedExprTable(_) => false
                                            val shapeSugar: Bool = exprCount == 5 && armCount == 3 && rootCore == 4 && exprs(4) is SelectExpr(_, 0, CoreSpan(0, 3), _)
                                            val armsSugar: Bool = arms(0) is ValueArm(1, 3, 3) && arms(1) is ValueArm(7, 7, 2) && arms(2) is RestArm(1)
                                            validSugar && shapeSugar && armsSugar
                                          else false
  Stigi.Test.expect(allPass, true, "Stigi.Elaborate must lower a range arm and a literal arm into value arms")
