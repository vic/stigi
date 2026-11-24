// prompt: infer an omitted underscore call length argument from its paired string literal argument.
def Stigi.Elaborate.inferCallArgLength(tables: Stigi.Syntax.Tables, argsSpan: Stigi.Syntax.Span, lenArgPos: U64, bufArgPos: U64) <below(lenArgPos, 256), below(bufArgPos, 256)>: Stigi.Elaborate.LengthInfer =
  val Tables(_, _, _, synLists, _, _) = tables
  argsSpan match
    case Span(first, endPos) =>
      val lenAt: U64 = first + lenArgPos
      val bufAt: U64 = first + bufArgPos
      if lenAt < endPos && bufAt < endPos then
        val lenSyn: Stigi.Syntax.NodeAt = synLists(lenAt)
        val bufSyn: Stigi.Syntax.NodeAt = synLists(bufAt)
        Stigi.Elaborate.decodeInferredString(tables, lenSyn, bufSyn)
      else LengthInferInvalid
