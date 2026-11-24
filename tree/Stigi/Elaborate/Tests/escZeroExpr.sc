// prompt: verify the character literal zero escape in an expression is zero.
val Stigi.Elaborate.Tests.escZeroExpr {Stigi.Test}: Stigi.Test.Result =
  val srcText: Array[U8](58) = "// prompt: pick demo.\ndef Demo.pick(x: U64): U64 =\n  '\\0'\n"
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
                              Stigi.Syntax.Parse.parseUnit(tables, 58, srcText, 58) match
                                case ParseRejected(_, _) => false
                                case LexRejected(_) => false
                                case Parsed(rootNode, _, _) =>
                                  Stigi.Elaborate.lowerUnit(tables, rootNode, CoreTables(16, exprs, 0, 0, 8, childLists, 0, 0, 8, arms, 0, 0), WorkScratch(128, work, 0, mapped, patRes)) match
                                    case LowerOverflow => false
                                    case LowerInvalid => false
                                    case Lowered(_, CoreTables(_, _, _, _, _, _, _, _, _, _, _, _), _) => exprs(0) is IntExpr(_, 0, _, _)
  Stigi.Test.expect(allPass, true, "Stigi.Elaborate must handle the character literal zero escape in an expression is zero as stated")
