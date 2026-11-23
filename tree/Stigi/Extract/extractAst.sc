// prompt: walk a parsed flat AST node arena and extract SSA order, call, allocation, and capability facts.
pub def Stigi.Extract.extractAst {Stigi.Mut} (tokCap: Stigi.TokenCap, toks: Array[Stigi.Lex.Token](tokCap), tokCount: U64, nodeCap: Stigi.NodeCap, nodes: Array[Stigi.Ast.Node](nodeCap), nodeCount: U64, factCap: Stigi.NodeCap, outFacts: Array[Stigi.Extract.FactRow](factCap)) <lessOrEqual(tokCount, tokCap), lessOrEqual(nodeCount, nodeCap)>: U64 <lessOrEqual(result, factCap)> =
  var idx: U64 = 0
  var factCount: U64 = 0
  while idx < nodeCount && factCount < factCap do
    @<lessOrEqual(idx, nodeCount), lessOrEqual(factCount, factCap)>
    val Node(kind, tokIdx, leftChild, rightChild, _, _) = nodes(idx)
    if kind == ExprBin && tokIdx < tokCount then
      val Token(operKind, _, _, _, _) = toks(tokIdx)
      if operKind == TokLess then
        outFacts(factCount) := FactRow(FactBelow, leftChild, rightChild, idx)
        factCount = factCount + 1
      else if operKind == TokLessEqual then
        outFacts(factCount) := FactRow(FactLessEqual, leftChild, rightChild, idx)
        factCount = factCount + 1
      else ()
    else if kind == ExprCall then
      outFacts(factCount) := FactRow(FactCall, idx, leftChild, rightChild)
      factCount = factCount + 1
    else if kind == ExprVal || kind == ExprVar then
      outFacts(factCount) := FactRow(FactStackObject, idx, leftChild, 0)
      factCount = factCount + 1
    else if kind == ExprHandle then
      outFacts(factCount) := FactRow(FactCapState, idx, leftChild, 0)
      factCount = factCount + 1
    else ()
    idx = idx + 1
  factCount
