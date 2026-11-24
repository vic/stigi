// prompt: verify a guarded last arm, which has no following arm to fall to, is rejected by lowering.
val Stigi.Elaborate.Tests.guardRejects {Stigi.Test}: Stigi.Test.Result =
  val srcLast: Array[U8](128) = "// prompt: pick demo.\ndef Demo.pick(x: Demo.Pick): U64 =\n  x match\n    case One => 1\n    case Two => 2\n    case other if x => 3\n"
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
                                  Stigi.Syntax.Parse.parseUnit(tables, 128, srcLast, 128) match
                                    case ParseRejected(_, _) => false
                                    case LexRejected(_) => false
                                    case Parsed(rootLast, _, _) =>
                                      Stigi.Elaborate.lowerUnit(tables, rootLast, 16, exprs, 8, childLists, 8, arms, 64, workNode, workPhase, 128, mapped, patRes) match
                                        case LowerOverflow => false
                                        case LowerInvalid => true
                                        case Lowered(_, _, _, _) => false
  Stigi.Test.expect(allPass, true, "Stigi.Elaborate must reject a guarded last arm")
