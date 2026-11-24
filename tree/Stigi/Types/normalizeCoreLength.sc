// prompt: iteratively normalize a Checked-IR expression subtree at exprAt into a signed Linear.Form using explicit work stacks.
def Stigi.Types.normalizeCoreLength {Stigi.Mut} (core: Stigi.Core.Tables, context: Stigi.Types.LengthContext, exprAt: U64, nodeStack: Array[U64](64), stepStack: Array[U64](64), formStack: Array[Stigi.Types.Linear.Form](64)) <below(exprAt, Stigi.Limit.maxUnitNodes)>: Stigi.Types.LengthOutcome =
  core match
    case CoreTables(expressionCapacity, expressions, _, expressionEnd, _, _, _, _, _, _, _, _) =>
      context match
        case LengthSpace(poolCapacity, pool, poolEnd, binderCapacity, binders, globalCapacity, globals, _, _, _) =>
          nodeStack(0) := exprAt
          stepStack(0) := 0
          var workTop: U64 = 1
          var valTop: U64 = 0
          var curPool: U64 = poolEnd
          var failed: Bool = false
          var failOutcome: Stigi.Types.LengthOutcome = LengthNotLinear
          while 0 < workTop && !failed do
            @<lessOrEqual(workTop, 64), lessOrEqual(valTop, 64), lessOrEqual(curPool, poolCapacity)>
            val frameIdx: U64 = workTop - 1
            val curExpr: U64 = nodeStack(frameIdx)
            val step: U64 = stepStack(frameIdx)
            if curExpr < expressionEnd then
              if step == 0 then
                expressions(curExpr) match
                  case IntExpr(_, lowWord, highWord, _) =>
                    if valTop < 64 then
                      formStack(valTop) := LinearForm(WideInt(lowWord, highWord), curPool, 0)
                      valTop = valTop + 1
                      workTop = frameIdx
                    else
                      failed = true
                      failOutcome = LengthPoolFull
                  case LocalExpr(_, binderSlot, _) =>
                    if binderSlot < binderCapacity then
                      Stigi.Types.resolveBinderKind(poolCapacity, pool, curPool, binderSlot, binders(binderSlot)) match
                        case LengthValue(resolvedForm, nextPool) =>
                          if valTop < 64 && nextPool <= poolCapacity then
                            formStack(valTop) := resolvedForm
                            valTop = valTop + 1
                            curPool = nextPool
                            workTop = frameIdx
                          else
                            failed = true
                            failOutcome = LengthPoolFull
                        case LengthNotLinear =>
                          failed = true
                          failOutcome = LengthNotLinear
                        case LengthOverflow =>
                          failed = true
                          failOutcome = LengthOverflow
                        case LengthDivZero =>
                          failed = true
                          failOutcome = LengthDivZero
                        case LengthPoolFull =>
                          failed = true
                          failOutcome = LengthPoolFull
                        case LengthNotUnsigned =>
                          failed = true
                          failOutcome = LengthNotUnsigned
                    else
                      failed = true
                      failOutcome = LengthNotLinear
                  case GlobalExpr(_, refVal, _) =>
                    refVal match
                      case ExternalRef(unitSlot, _) =>
                        if unitSlot < globalCapacity then
                          globals(unitSlot) match
                            case KnownBinder(knownForm) =>
                              if valTop < 64 then
                                formStack(valTop) := knownForm
                                valTop = valTop + 1
                                workTop = frameIdx
                              else
                                failed = true
                                failOutcome = LengthPoolFull
                            case ImmutableBinder | MutableBinder =>
                              failed = true
                              failOutcome = LengthNotLinear
                        else
                          failed = true
                          failOutcome = LengthNotLinear
                      case SelfGroupRef(_) | InnerRef(_) =>
                        failed = true
                        failOutcome = LengthNotLinear
                  case ExactExpr(at, _, leftAt, _, _) =>
                    if at == curExpr then
                      if workTop < 64 then
                        stepStack(frameIdx) := 1
                        nodeStack(workTop) := leftAt
                        stepStack(workTop) := 0
                        workTop = workTop + 1
                      else
                        failed = true
                        failOutcome = LengthPoolFull
                    else
                      failed = true
                      failOutcome = LengthNotLinear
                  case NegateExpr(at, operandAt) =>
                    if at == curExpr then
                      if workTop < 64 then
                        stepStack(frameIdx) := 3
                        nodeStack(workTop) := operandAt
                        stepStack(workTop) := 0
                        workTop = workTop + 1
                      else
                        failed = true
                        failOutcome = LengthPoolFull
                    else
                      failed = true
                      failOutcome = LengthNotLinear
                  case TruthExpr(_, _) | NothingExpr(_) | TextExpr(_, _, _) | ConstructExpr(_, _, _, _, _) | CallExpr(_, _, _, _, _) | OperCallExpr(_, _, _, _, _) | CompareExpr(_, _, _, _) | BothExpr(_, _, _) | EitherExpr(_, _, _) | ChoiceExpr(_, _, _, _, _) | LoopExpr(_, _, _, _, _) | SelectExpr(_, _, _, _) | BindExpr(_, _, _, _, _) | MutableExpr(_, _, _, _, _, _) | AssignExpr(_, _, _) | ElemReadExpr(_, _, _, _) | ElemWriteExpr(_, _, _, _) | HandleExpr(_, _, _, _, _, _) | AssertExpr(_, _, _, _) | AssertValExpr(_, _, _, _) | DenyExpr(_, _, _, _) =>
                    failed = true
                    failOutcome = LengthNotLinear
              else if step == 1 then
                expressions(curExpr) match
                  case ExactExpr(_, _, _, rightAt, _) =>
                    if workTop < 64 then
                      stepStack(frameIdx) := 2
                      nodeStack(workTop) := rightAt
                      stepStack(workTop) := 0
                      workTop = workTop + 1
                    else
                      failed = true
                      failOutcome = LengthPoolFull
                  case IntExpr(_, _, _, _) | LocalExpr(_, _, _) | GlobalExpr(_, _, _) | NegateExpr(_, _) | TruthExpr(_, _) | NothingExpr(_) | TextExpr(_, _, _) | ConstructExpr(_, _, _, _, _) | CallExpr(_, _, _, _, _) | OperCallExpr(_, _, _, _, _) | CompareExpr(_, _, _, _) | BothExpr(_, _, _) | EitherExpr(_, _, _) | ChoiceExpr(_, _, _, _, _) | LoopExpr(_, _, _, _, _) | SelectExpr(_, _, _, _) | BindExpr(_, _, _, _, _) | MutableExpr(_, _, _, _, _, _) | AssignExpr(_, _, _) | ElemReadExpr(_, _, _, _) | ElemWriteExpr(_, _, _, _) | HandleExpr(_, _, _, _, _, _) | AssertExpr(_, _, _, _) | AssertValExpr(_, _, _, _) | DenyExpr(_, _, _, _) =>
                    failed = true
                    failOutcome = LengthNotLinear
              else if step == 2 then
                expressions(curExpr) match
                  case ExactExpr(_, arithKind, _, _, _) =>
                    if 2 <= valTop then
                      val leftForm: Stigi.Types.Linear.Form = formStack(valTop - 2)
                      val rightForm: Stigi.Types.Linear.Form = formStack(valTop - 1)
                      val baseTop: U64 = valTop - 2
                      Stigi.Types.applyLengthBinary(poolCapacity, pool, arithKind, leftForm, rightForm, curPool) match
                        case LengthValue(resForm, nextPool) =>
                          if nextPool <= poolCapacity then
                            formStack(baseTop) := resForm
                            valTop = baseTop + 1
                            curPool = nextPool
                            workTop = frameIdx
                          else
                            failed = true
                            failOutcome = LengthPoolFull
                        case LengthNotLinear =>
                          failed = true
                          failOutcome = LengthNotLinear
                        case LengthOverflow =>
                          failed = true
                          failOutcome = LengthOverflow
                        case LengthDivZero =>
                          failed = true
                          failOutcome = LengthDivZero
                        case LengthPoolFull =>
                          failed = true
                          failOutcome = LengthPoolFull
                        case LengthNotUnsigned =>
                          failed = true
                          failOutcome = LengthNotUnsigned
                    else
                      failed = true
                      failOutcome = LengthNotLinear
                  case IntExpr(_, _, _, _) | LocalExpr(_, _, _) | GlobalExpr(_, _, _) | NegateExpr(_, _) | TruthExpr(_, _) | NothingExpr(_) | TextExpr(_, _, _) | ConstructExpr(_, _, _, _, _) | CallExpr(_, _, _, _, _) | OperCallExpr(_, _, _, _, _) | CompareExpr(_, _, _, _) | BothExpr(_, _, _) | EitherExpr(_, _, _) | ChoiceExpr(_, _, _, _, _) | LoopExpr(_, _, _, _, _) | SelectExpr(_, _, _, _) | BindExpr(_, _, _, _, _) | MutableExpr(_, _, _, _, _, _) | AssignExpr(_, _, _) | ElemReadExpr(_, _, _, _) | ElemWriteExpr(_, _, _, _) | HandleExpr(_, _, _, _, _, _) | AssertExpr(_, _, _, _) | AssertValExpr(_, _, _, _) | DenyExpr(_, _, _, _) =>
                    failed = true
                    failOutcome = LengthNotLinear
              else
                if 1 <= valTop then
                  val innerForm: Stigi.Types.Linear.Form = formStack(valTop - 1)
                  val baseTop: U64 = valTop - 1
                  val negOne: Stigi.Types.Linear.Form = LinearForm(WideInt(18446744073709551615, -1), curPool, 0)
                  Stigi.Types.mapLinearOutcome(Stigi.Types.Linear.multiply(poolCapacity, pool, negOne, innerForm, curPool)) match
                    case LengthValue(resForm, nextPool) =>
                      if nextPool <= poolCapacity then
                        formStack(baseTop) := resForm
                        valTop = baseTop + 1
                        curPool = nextPool
                        workTop = frameIdx
                      else
                        failed = true
                        failOutcome = LengthPoolFull
                    case LengthNotLinear =>
                      failed = true
                      failOutcome = LengthNotLinear
                    case LengthOverflow =>
                      failed = true
                      failOutcome = LengthOverflow
                    case LengthDivZero =>
                      failed = true
                      failOutcome = LengthDivZero
                    case LengthPoolFull =>
                      failed = true
                      failOutcome = LengthPoolFull
                    case LengthNotUnsigned =>
                      failed = true
                      failOutcome = LengthNotUnsigned
                else
                  failed = true
                  failOutcome = LengthNotLinear
            else
              failed = true
              failOutcome = LengthNotLinear
          if failed then failOutcome else if valTop == 1 then LengthValue(formStack(0), curPool) else LengthNotLinear
