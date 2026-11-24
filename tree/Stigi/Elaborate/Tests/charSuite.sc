// prompt: verify character literal arms lower into value arms holding the decoded byte, never the quote.
val Stigi.Elaborate.Tests.charSuite {Stigi.Test}: Stigi.Test.Result =
  val srcChar: Array[U8](118) = "// prompt: pick demo.\ndef Demo.pick(x: U64): U64 =\n  x match\n    case 'a' => 1\n    case '\\n' => 2\n    case other => 3\n"
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
                              Stigi.Syntax.Parse.parseUnit(tables, 118, srcChar, 118) match
                                case ParseRejected(_, _) => false
                                case LexRejected(_) => false
                                case Parsed(rootChar, _, _) =>
                                  Stigi.Elaborate.lowerUnit(tables, rootChar, CoreTables(16, exprs, 0, 0, 8, childLists, 0, 0, 8, arms, 0, 0), WorkScratch(128, work, 0, mapped, patRes)) match
                                    case LowerOverflow => false
                                    case LowerInvalid => false
                                    case Lowered(_, CoreTables(_, _, _, _, _, _, _, _, _, _, _, armCount), _) => armCount == 3 && arms(0) is ValueArm(97, 97, _) && arms(1) is ValueArm(10, 10, _) && arms(2) is RestArm(_)
  Stigi.Test.expect(allPass, true, "Stigi.Elaborate must lower a character arm and an escape arm into value arms holding the decoded byte")
