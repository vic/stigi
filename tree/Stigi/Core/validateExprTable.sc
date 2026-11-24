// prompt: validate postorder index invariants for every expression in a flat Checked-IR table.
pub def Stigi.Core.validateExprTable(nodeCap: U64, exprs: Array[Stigi.Core.Expr](nodeCap), exprCount: U64, armCap: U64, arms: Array[Stigi.Core.Arm](armCap), armCount: U64, listCap: U64, childLists: Array[U64](listCap), listCount: U64) <lessOrEqual(exprCount, nodeCap), lessOrEqual(armCount, armCap), lessOrEqual(listCount, listCap)>: Stigi.Core.TableCheck =
  var idx: U64 = 0
  var verdict: Stigi.Core.TableCheck = TableValid(exprCount)
  var failed: Bool = false
  while idx < exprCount && !failed do
    @<lessOrEqual(idx, exprCount)>
    val nextIdx: U64 = idx + 1
    val curExpr: Stigi.Core.Expr = exprs(idx)
    val nodeAt: U64 = Stigi.Core.exprAtOf(curExpr)
    if nodeAt != idx then
      verdict = MalformedTable(idx)
      failed = true
    else
      val entryGood: Bool = curExpr match
        case LocalExpr(_, _, _) => true
        case GlobalExpr(_, _, _) => true
        case IntExpr(_, _, _, _) => true
        case TruthExpr(_, _) => true
        case NothingExpr(_) => true
        case TextExpr(_, _, _) => true
        case ConstructExpr(_, _, _, argSpan, _) => Stigi.Core.validateChildSpan(nodeAt, listCap, childLists, listCount, argSpan)
        case CallExpr(_, _, argSpan, _, _) => Stigi.Core.validateChildSpan(nodeAt, listCap, childLists, listCount, argSpan)
        case OperCallExpr(_, _, _, argSpan, _) => Stigi.Core.validateChildSpan(nodeAt, listCap, childLists, listCount, argSpan)
        case ExactExpr(_, _, _, _, _) => true
        case CompareExpr(_, _, _, _) => true
        case BothExpr(_, _, _) => true
        case EitherExpr(_, _, _) => true
        case NegateExpr(_, _) => true
        case ChoiceExpr(_, _, _, _, _) => true
        case LoopExpr(_, _, _, meas, _) =>
          meas match
            case Unmeasured => true
            case DecreasesExpr(measAt) => measAt < nodeAt
        case SelectExpr(_, _, armsSpan, _) => Stigi.Core.validateArmSpan(nodeAt, armCap, arms, armCount, armsSpan)
        case BindExpr(_, _, _, _, _) => true
        case MutableExpr(_, _, _, _, _, _) => true
        case AssignExpr(_, _, _) => true
        case ElemReadExpr(_, _, _, _) => true
        case ElemWriteExpr(_, _, _, _) => true
        case HandleExpr(_, _, _, argSpan, _, _) => Stigi.Core.validateChildSpan(nodeAt, listCap, childLists, listCount, argSpan)
        case AssertExpr(_, _, _, _) => true
        case AssertValExpr(_, _, _, _) => true
        case DenyExpr(_, _, _, _) => true
      if !entryGood then
        verdict = MalformedTable(idx)
        failed = true
      else
        ()
    idx = nextIdx
  verdict
