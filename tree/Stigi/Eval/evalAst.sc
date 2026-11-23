// prompt: evaluate a parsed flat AST expression arena iteratively for comptime folding.
def Stigi.Eval.evalAst {region: Stigi.Region, Stigi.Mut} (srcCap: Stigi.ByteCap, src: Array[U8](srcCap), tokCap: Stigi.TokenCap, toks: Array[Stigi.Lex.Token](tokCap), tokCount: U64, nodeCap: Stigi.NodeCap, nodes: Array[Stigi.Ast.Node](nodeCap), nodeCount: U64, rootIdx: U64) <lessOrEqual(tokCount, tokCap), lessOrEqual(nodeCount, nodeCap), below(rootIdx, nodeCount)>: Stigi.Eval.Value =
  val Filled(vals) = Array.tryFill[Stigi.Eval.Value]{region}(nodeCap, ValDynamic) else
    case FillFailed => ValOverflow
  var idx: U64 = 0
  while idx < nodeCount do
    @<lessOrEqual(idx, nodeCount)>
    val Node(kind, tokIdx, leftChild, rightChild, _, _) = nodes(idx)
    if kind == ExprInt && tokIdx < tokCount then
      val Token(_, beg, fin, _, _) = toks(tokIdx)
      if fin <= srcCap then vals(idx) := Stigi.Eval.parseUintSlice(srcCap, src, beg, fin) else ()
    else if kind == ExprBool && tokIdx < tokCount then
      val Token(tokKind, _, _, _, _) = toks(tokIdx)
      vals(idx) := ValFlag(tokKind == TokTrue)
    else if kind == ExprBin && tokIdx < tokCount && leftChild < idx && rightChild < idx then
      val Token(operKind, _, _, _, _) = toks(tokIdx)
      val leftVal: Stigi.Eval.Value = vals(leftChild)
      val rightVal: Stigi.Eval.Value = vals(rightChild)
      vals(idx) := leftVal match
        case ValWord(leftNum) =>
          rightVal match
            case ValWord(rightNum) =>
              if operKind == TokPlus && leftNum < 1000000000000000 && rightNum < 1000000000000000 then ValWord(leftNum + rightNum) else if operKind == TokMinus && rightNum <= leftNum then ValWord(leftNum - rightNum) else if operKind == TokStar && leftNum < 100000000 && rightNum < 100000000 then ValWord(leftNum * rightNum) else if operKind == TokEqual then ValFlag(leftNum == rightNum) else if operKind == TokLess then ValFlag(leftNum < rightNum) else ValDynamic
            case ValFlag(_) => ValDynamic
            case ValDynamic => ValDynamic
            case ValOverflow => ValOverflow
        case ValFlag(_) => ValDynamic
        case ValDynamic => ValDynamic
        case ValOverflow => ValOverflow
    else ()
    idx = idx + 1
  vals(rootIdx)
