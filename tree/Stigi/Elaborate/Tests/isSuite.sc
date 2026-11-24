// prompt: verify real parsed is-test lowering into one select node with a pattern arm and a rest arm, and wildcard is-test lowering reusing one rest arm at exact arm capacity.
val Stigi.Elaborate.Tests.isSuite {Stigi.Test}: Stigi.Test.Result =
  val srcIs: Array[U8](74) = "// prompt: check is test.\ndef Demo.check(x: Demo.Pick): Bool =\n  x is One\n"
  val srcWild: Array[U8](81) = "// prompt: check wildcard is test.\ndef Demo.check(x: Demo.Pick): Bool =\n  x is _\n"
  val caseRes: Stigi.Elaborate.PatResolution = PatCaseOrd(2)
  val zeroWord: U64 = 0
  val initWork: Stigi.Elaborate.Work = Enter(0)
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
              Array.tryFill{Stigi.Region}(1, initArm) match
                case FillFailed => false
                case Filled(oneArm) =>
                  Array.tryFill{Stigi.Region}(8, zeroWord) match
                    case FillFailed => false
                    case Filled(childLists) =>
                      Array.tryFill{Stigi.Region}(128, initWork) match
                        case FillFailed => false
                        case Filled(work) =>
                          Array.tryFill{Stigi.Region}(128, zeroWord) match
                            case FillFailed => false
                            case Filled(mapped) =>
                              Array.tryFill{Stigi.Region}(128, caseRes) match
                                case FillFailed => false
                                case Filled(patCase) =>
                                  Array.tryFill{Stigi.Region}(128, PatLocalBind) match
                                    case FillFailed => false
                                    case Filled(patWild) =>
                                      val passCase: Bool = Stigi.Syntax.Parse.parseUnit(tables, 74, srcIs, 74) match
                                        case ParseRejected(_, _) => false
                                        case LexRejected(_) => false
                                        case Parsed(rootIs, _, _) =>
                                          Stigi.Elaborate.lowerUnit(tables, rootIs, CoreTables(16, exprs, 0, 0, 8, childLists, 0, 0, 4, arms, 0, 0), WorkScratch(128, work, 0, mapped, patCase)) match
                                            case LowerOverflow => false
                                            case LowerInvalid => false
                                            case Lowered(rootCore, CoreTables(_, _, _, exprCount, _, _, _, listCount, _, _, _, armCount), _) =>
                                              if exprCount <= 16 && armCount <= 4 && listCount <= 8 then
                                                val validIs: Bool = Stigi.Core.validateExprTable(16, exprs, exprCount, 4, arms, armCount, 8, childLists, listCount) match
                                                  case TableValid(cnt) => cnt == exprCount
                                                  case MalformedExprTable(_) => false
                                                val shapeIs: Bool = exprCount == 4 && armCount == 2 && rootCore == 3 && exprs(0) is LocalExpr(_, _, _) && exprs(1) is TruthExpr(_, true) && exprs(2) is TruthExpr(_, false) && exprs(3) is SelectExpr(_, 0, CoreSpan(0, 2), _)
                                                val armsIs: Bool = arms(0) is CaseArm(2, _, 1) && arms(1) is RestArm(2)
                                                validIs && shapeIs && armsIs
                                              else false
                                      val passWild: Bool = Stigi.Syntax.Parse.parseUnit(tables, 81, srcWild, 81) match
                                        case ParseRejected(_, _) => false
                                        case LexRejected(_) => false
                                        case Parsed(rootWild, _, _) =>
                                          Stigi.Elaborate.lowerUnit(tables, rootWild, CoreTables(16, exprs, 0, 0, 8, childLists, 0, 0, 1, oneArm, 0, 0), WorkScratch(128, work, 0, mapped, patWild)) match
                                            case LowerOverflow => false
                                            case LowerInvalid => false
                                            case Lowered(rootCore, CoreTables(_, _, _, exprCount, _, _, _, listCount, _, _, _, armCount), _) =>
                                              if exprCount <= 16 && armCount <= 1 && listCount <= 8 then
                                                val validWild: Bool = Stigi.Core.validateExprTable(16, exprs, exprCount, 1, oneArm, armCount, 8, childLists, listCount) match
                                                  case TableValid(cnt) => cnt == exprCount
                                                  case MalformedExprTable(_) => false
                                                val shapeWild: Bool = exprCount == 3 && armCount == 1 && rootCore == 2 && exprs(0) is LocalExpr(_, _, _) && exprs(1) is TruthExpr(_, true) && exprs(2) is SelectExpr(_, 0, CoreSpan(0, 1), _)
                                                val armWild: Bool = oneArm(0) is RestArm(1)
                                                validWild && shapeWild && armWild
                                              else false
                                      passCase && passWild
  Stigi.Test.expect(allPass, true, "Stigi.Elaborate must lower an is-test into a select node with a pattern arm and a rest arm")
