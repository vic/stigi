// prompt: represent all 26 postorder expression kinds of the Checked IR with child-below-parent facts.
pub enum Stigi.Core.Expr:
  case LocalExpr(at: U64, binderSlot: U64, typeSlot: U64)
  case GlobalExpr(at: U64, refVal: Stigi.Core.Reference, typeSlot: U64)
  case IntExpr(at: U64, lowWord: U64, highWord: I64, typeSlot: U64)
  case TruthExpr(at: U64, flag: Bool)
  case NothingExpr(at: U64)
  case TextExpr(at: U64, textSpan: Stigi.Core.Span, typeSlot: U64)
  case ConstructExpr(at: U64, enumRef: Stigi.Core.Reference, caseIndex: U64, args: Stigi.Core.Span, typeSlot: U64)
  case CallExpr(at: U64, callee: Stigi.Core.Reference, args: Stigi.Core.Span, callKind: Stigi.Core.CallKind, typeSlot: U64)
  case OperCallExpr(at: U64, capBinder: U64, operIndex: U64, args: Stigi.Core.Span, typeSlot: U64)
  case ExactExpr(at: U64, arithKind: Stigi.Core.ArithKind, leftAt: U64, rightAt: U64, typeSlot: U64) <below(leftAt, at), below(rightAt, at)>
  case CompareExpr(at: U64, cmpKind: Stigi.Core.CompareKind, leftAt: U64, rightAt: U64) <below(leftAt, at), below(rightAt, at)>
  case BothExpr(at: U64, leftAt: U64, rightAt: U64) <below(leftAt, at), below(rightAt, at)>
  case EitherExpr(at: U64, leftAt: U64, rightAt: U64) <below(leftAt, at), below(rightAt, at)>
  case NegateExpr(at: U64, operandAt: U64) <below(operandAt, at)>
  case ChoiceExpr(at: U64, condAt: U64, thenAt: U64, elseAt: U64, typeSlot: U64) <below(condAt, at), below(thenAt, at), below(elseAt, at)>
  case LoopExpr(at: U64, condAt: U64, invFacts: Stigi.Core.Span, measure: Stigi.Core.Measure, bodyAt: U64) <below(condAt, at), below(bodyAt, at)>
  case SelectExpr(at: U64, scrutAt: U64, arms: Stigi.Core.Span, typeSlot: U64) <below(scrutAt, at)>
  case BindExpr(at: U64, binderSlot: U64, initAt: U64, restAt: U64, typeSlot: U64) <below(initAt, at), below(restAt, at)>
  case MutableExpr(at: U64, binderSlot: U64, initAt: U64, invFacts: Stigi.Core.Span, restAt: U64, typeSlot: U64) <below(initAt, at), below(restAt, at)>
  case AssignExpr(at: U64, binderSlot: U64, valueAt: U64) <below(valueAt, at)>
  case ElemReadExpr(at: U64, targetAt: U64, indexAt: U64, typeSlot: U64) <below(targetAt, at), below(indexAt, at)>
  case ElemWriteExpr(at: U64, targetAt: U64, indexAt: U64, valueAt: U64) <below(targetAt, at), below(indexAt, at), below(valueAt, at)>
  case HandleExpr(at: U64, binderSlot: U64, handlerSlot: U64, args: Stigi.Core.Span, restAt: U64, typeSlot: U64) <below(restAt, at)>
  case AssertExpr(at: U64, facts: Stigi.Core.Span, restAt: U64, typeSlot: U64) <below(restAt, at)>
  case AssertValExpr(at: U64, valueAt: U64, facts: Stigi.Core.Span, typeSlot: U64) <below(valueAt, at)>
  case DenyExpr(at: U64, atoms: Stigi.Core.Span, restAt: U64, typeSlot: U64) <below(restAt, at)>
