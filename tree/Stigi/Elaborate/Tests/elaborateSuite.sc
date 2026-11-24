// prompt: verify real Stigi.Syntax.Parse.parseUnit lowering into Stigi.Core postorder tables, comparison swap, pipe desugaring, and malformed table rejection.
val Stigi.Elaborate.Tests.elaborateSuite {Stigi.Test}: Stigi.Test.Result =
  val srcCmp: Array[U8](102) = "// prompt: check greater comparison.\ndef Demo.check(left: U64, right: U64): Bool =\n  left > right + 1\n"
  val srcPipe: Array[U8](84) = "// prompt: check pipe call.\ndef Demo.pipe(valIn: U64): U64 =\n  valIn |> Demo.add(1)\n"
  val zeroWord: U64 = 0
  val zeroByte: U8 = U8.ofU64(0)
  val initExpr: Stigi.Core.Expr = NothingExpr(0)
  val initArm: Stigi.Core.Arm = RestArm(0)
  val allPass: Bool = Stigi.Syntax.allocate{Stigi.Region}() match
    case AllocationFailed => false
    case Allocated(tables) =>
      Array.tryFill{Stigi.Region}(64, initExpr) match
        case FillFailed => false
        case Filled(exprs) =>
          Array.tryFill{Stigi.Region}(16, initArm) match
            case FillFailed => false
            case Filled(arms) =>
              Array.tryFill{Stigi.Region}(64, zeroWord) match
                case FillFailed => false
                case Filled(childLists) =>
                  Array.tryFill{Stigi.Region}(128, zeroWord) match
                    case FillFailed => false
                    case Filled(workNode) =>
                      Array.tryFill{Stigi.Region}(128, zeroByte) match
                        case FillFailed => false
                        case Filled(workPhase) =>
                          Array.tryFill{Stigi.Region}(128, zeroWord) match
                            case FillFailed => false
                            case Filled(mapped) =>
                              val chkCmp: Bool = Stigi.Syntax.Parse.parseUnit(tables, 102, srcCmp, 102) match
                                case ParseRejected(_, _) => false
                                case LexRejected(_) => false
                                case Parsed(rootCmp, _, _) =>
                                  Stigi.Elaborate.lowerUnit(tables, rootCmp, 64, exprs, 64, childLists, 128, workNode, workPhase, 128, mapped) match
                                    case LowerOverflow => false
                                    case LowerInvalid => false
                                    case Lowered(rootCore, exprCount, listCount) =>
                                      if exprCount <= 64 && listCount <= 64 && rootCore < 64 then
                                        val validFlag: Bool = Stigi.Core.validateExprTable(64, exprs, exprCount, 16, arms, 0, 64, childLists, listCount) match
                                          case TableValid(cnt) => cnt == exprCount
                                          case MalformedTable(_) => false
                                        val swapFlag: Bool = exprs(rootCore) match
                                          case CompareExpr(_, cmpKind, leftAt, _) =>
                                            val isLess: Bool = cmpKind match
                                              case CoreLess => true
                                              case CoreLessOrEqual => false
                                              case CoreEqual => false
                                              case CoreNotEqual => false
                                            val leftIsAdd: Bool = if leftAt < 64 then exprs(leftAt) is ExactExpr(_, CoreAdd, _, _, _) else false
                                            isLess && leftIsAdd
                                          case LocalExpr(_, _, _) | GlobalExpr(_, _, _) | IntExpr(_, _, _, _) | TruthExpr(_, _) | NothingExpr(_) | TextExpr(_, _, _) | ConstructExpr(_, _, _, _, _) | CallExpr(_, _, _, _, _) | OperCallExpr(_, _, _, _, _) | ExactExpr(_, _, _, _, _) | BothExpr(_, _, _) | EitherExpr(_, _, _) | NegateExpr(_, _) | ChoiceExpr(_, _, _, _, _) | LoopExpr(_, _, _, _, _) | SelectExpr(_, _, _, _) | BindExpr(_, _, _, _, _) | MutableExpr(_, _, _, _, _, _) | AssignExpr(_, _, _) | ElemReadExpr(_, _, _, _) | ElemWriteExpr(_, _, _, _) | HandleExpr(_, _, _, _, _, _) | AssertExpr(_, _, _, _) | AssertValExpr(_, _, _, _) | DenyExpr(_, _, _, _) => false
                                        validFlag && swapFlag
                                      else false
                              val chkPipe: Bool = Stigi.Syntax.Parse.parseUnit(tables, 84, srcPipe, 84) match
                                case ParseRejected(_, _) => false
                                case LexRejected(_) => false
                                case Parsed(rootPipe, _, _) =>
                                  Stigi.Elaborate.lowerUnit(tables, rootPipe, 64, exprs, 64, childLists, 128, workNode, workPhase, 128, mapped) match
                                    case LowerOverflow => false
                                    case LowerInvalid => false
                                    case Lowered(rootCore, exprCount, listCount) =>
                                      if exprCount <= 64 && listCount <= 64 && rootCore < 64 then
                                        val validPipe: Bool = Stigi.Core.validateExprTable(64, exprs, exprCount, 16, arms, 0, 64, childLists, listCount) match
                                          case TableValid(cnt) => cnt == exprCount
                                          case MalformedTable(_) => false
                                        val callGood: Bool = exprs(rootCore) match
                                          case CallExpr(_, _, _, TailCall, _) => false
                                          case CallExpr(_, _, argSpan, NormalCall, _) =>
                                            val CoreSpan(argFirst, argEnd) = argSpan
                                            if argEnd - argFirst == 2 && argFirst < 64 && argFirst + 1 < 64 then
                                              val firstIdx: U64 = childLists(argFirst)
                                              val secondIdx: U64 = childLists(argFirst + 1)
                                              val firstLocal: Bool = if firstIdx < 64 then exprs(firstIdx) is LocalExpr(_, _, _) else false
                                              val secondOne: Bool = if secondIdx < 64 then exprs(secondIdx) is IntExpr(_, 1, 0, _) else false
                                              firstLocal && secondOne
                                            else false
                                          case LocalExpr(_, _, _) | GlobalExpr(_, _, _) | IntExpr(_, _, _, _) | TruthExpr(_, _) | NothingExpr(_) | TextExpr(_, _, _) | ConstructExpr(_, _, _, _, _) | OperCallExpr(_, _, _, _, _) | ExactExpr(_, _, _, _, _) | CompareExpr(_, _, _, _) | BothExpr(_, _, _) | EitherExpr(_, _, _) | NegateExpr(_, _) | ChoiceExpr(_, _, _, _, _) | LoopExpr(_, _, _, _, _) | SelectExpr(_, _, _, _) | BindExpr(_, _, _, _, _) | MutableExpr(_, _, _, _, _, _) | AssignExpr(_, _, _) | ElemReadExpr(_, _, _, _) | ElemWriteExpr(_, _, _, _) | HandleExpr(_, _, _, _, _, _) | AssertExpr(_, _, _, _) | AssertValExpr(_, _, _, _) | DenyExpr(_, _, _, _) => false
                                        validPipe && callGood
                                      else false
                              exprs(0) := IntExpr(0, 1, 0, 0)
                              exprs(1) := CallExpr(1, SelfGroupRef(0), CoreSpan(0, 1), NormalCall, 0)
                              childLists(0) := 1
                              val chkBad: Bool = Stigi.Core.validateExprTable(64, exprs, 2, 16, arms, 0, 64, childLists, 1) match
                                case MalformedTable(badIdx) => badIdx == 1
                                case TableValid(_) => false
                              chkCmp && chkPipe && chkBad
  Stigi.Test.expect(allPass, true, "Stigi.Elaborate must lower real parsed units into valid Stigi.Core postorder tables")
