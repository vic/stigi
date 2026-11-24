// prompt: verify real parsed is-test lowering into one select node with a pattern arm and a rest arm.
val Stigi.Elaborate.Tests.isSuite {Stigi.Test}: Stigi.Test.Result =
  val srcIs: Array[U8](74) = "// prompt: check is test.\ndef Demo.check(x: Demo.Pick): Bool =\n  x is One\n"
  val caseRes: Stigi.Elaborate.PatResolution = PatCaseOrd(2)
  val zeroWord: U64 = 0
  val zeroByte: U8 = U8.ofU64(0)
  val initExpr: Stigi.Core.Expr = NothingExpr(0)
  val initArm: Stigi.Core.Arm = RestArm(0)
  val allPass: Bool = Stigi.Syntax.allocate{Stigi.Region}() match
    case AllocationFailed => false
    case Allocated(tables) =>
      Array.tryFill{Stigi.Region}(16, initExpr) match
        case FillFailed => false
        case Filled(exprs) =>
          Array.tryFill{Stigi.Region}(4, initArm) match
            case FillFailed => false
            case Filled(arms) =>
              Array.tryFill{Stigi.Region}(8, zeroWord) match
                case FillFailed => false
                case Filled(childLists) =>
                  Array.tryFill{Stigi.Region}(32, zeroWord) match
                    case FillFailed => false
                    case Filled(workNode) =>
                      Array.tryFill{Stigi.Region}(32, zeroByte) match
                        case FillFailed => false
                        case Filled(workPhase) =>
                          Array.tryFill{Stigi.Region}(128, zeroWord) match
                            case FillFailed => false
                            case Filled(mapped) =>
                              Array.tryFill{Stigi.Region}(128, caseRes) match
                                case FillFailed => false
                                case Filled(patCase) =>
                                  Stigi.Syntax.Parse.parseUnit(tables, 74, srcIs, 74) match
                                    case ParseRejected(_, _) => false
                                    case LexRejected(_) => false
                                    case Parsed(rootIs, _, _) =>
                                      Stigi.Elaborate.lowerUnit(tables, rootIs, 16, exprs, 8, childLists, 4, arms, 32, workNode, workPhase, 128, mapped, patCase) match
                                        case LowerOverflow => false
                                        case LowerInvalid => false
                                        case Lowered(rootCore, exprCount, armCount, listCount) =>
                                          if exprCount <= 16 && armCount <= 4 && listCount <= 8 then
                                            val validIs: Bool = Stigi.Core.validateExprTable(16, exprs, exprCount, 4, arms, armCount, 8, childLists, listCount) match
                                              case TableValid(cnt) => cnt == exprCount
                                              case MalformedExprTable(_) => false
                                            val shapeIs: Bool = exprCount == 4 && armCount == 2 && rootCore == 3 && exprs(0) is LocalExpr(_, _, _) && exprs(1) is TruthExpr(_, true) && exprs(2) is TruthExpr(_, false) && exprs(3) is SelectExpr(_, 0, CoreSpan(0, 2), _)
                                            val armsIs: Bool = arms(0) is CaseArm(2, _, 1) && arms(1) is RestArm(2)
                                            validIs && shapeIs && armsIs
                                          else false
  Stigi.Test.expect(allPass, true, "Stigi.Elaborate must lower an is-test into a select node with a pattern arm and a rest arm")
