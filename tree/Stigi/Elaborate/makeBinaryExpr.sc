// prompt: build the Checked-IR binary expression for a non-pipe operator, swapping operands for greater comparisons.
def Stigi.Elaborate.makeBinaryExpr(emitIdx: U64, operKind: Stigi.Syntax.OperKind, leftCore: U64, rightCore: U64) <below(leftCore, emitIdx), below(rightCore, emitIdx)>: Stigi.Core.Expr =
  operKind match
    case OperPlus => ExactExpr(emitIdx, CoreAdd, leftCore, rightCore, 0)
    case OperMinus => ExactExpr(emitIdx, CoreSub, leftCore, rightCore, 0)
    case OperTimes => ExactExpr(emitIdx, CoreMul, leftCore, rightCore, 0)
    case OperDivide => ExactExpr(emitIdx, CoreDiv, leftCore, rightCore, 0)
    case OperRem => ExactExpr(emitIdx, CoreRem, leftCore, rightCore, 0)
    case OperLess => CompareExpr(emitIdx, CoreLess, leftCore, rightCore)
    case OperAtMost => CompareExpr(emitIdx, CoreLessOrEqual, leftCore, rightCore)
    case OperGreater => CompareExpr(emitIdx, CoreLess, rightCore, leftCore)
    case OperAtLeast => CompareExpr(emitIdx, CoreLessOrEqual, rightCore, leftCore)
    case OperEquals => CompareExpr(emitIdx, CoreEqual, leftCore, rightCore)
    case OperDiffers => CompareExpr(emitIdx, CoreNotEqual, leftCore, rightCore)
    case OperAnd => BothExpr(emitIdx, leftCore, rightCore)
    case OperOr => EitherExpr(emitIdx, leftCore, rightCore)
    case OperPipe => NothingExpr(emitIdx)
