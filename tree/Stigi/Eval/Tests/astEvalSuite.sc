// prompt: verify primitive layouts, alignment law, and AST comptime evaluator folding.
val Stigi.Eval.Tests.astEvalSuite {Stigi.Test}: Stigi.Test.Result =
  Stigi.Types.Laws.alignMonotone(5, 8)
  val Layout(longSize, longAlign) = Stigi.Types.primLayout(PrimWordLong, 8)
  val alignedVal: U64 = Stigi.Types.alignOffset(5, 8)
  val exprSrc: Array[U8](32) = "// prompt: e.\nval E.x = 12 + 30\n"
  val evalValid: Bool =
    val Filled(toks) = Array.tryFill[Stigi.Lex.Token]{Stigi.Region}(32, Token(TokEnd, 0, 0, 0, 0)) else
      case FillFailed => false
    val Filled(nodes) = Array.tryFill[Stigi.Ast.Node]{Stigi.Region}(32, Node(ExprUnit, 0, 0, 0, 0, 0)) else
      case FillFailed => false
    val LexDone(tokCount) = Stigi.Lex.scanAll(32, exprSrc, 32, 32, toks) else
      case LexOverflow => false
      case LexSyntaxError(_, _) => false
    val ParsedUnit(header, nodeCount) = Stigi.Parse.parseUnit(32, exprSrc, 32, toks, tokCount, 32, nodes) else
      case ParseFailed(_) => false
    val UnitHeader(_, _, _, _, _, _, rootIdx) = header
    if rootIdx < nodeCount && nodeCount <= 32 then
      Stigi.Eval.evalAst{Stigi.Region}(32, exprSrc, 32, toks, tokCount, 32, nodes, nodeCount, rootIdx) match
        case ValWord(num) => num == 42
        case ValFlag(_) => false
        case ValDynamic => false
        case ValOverflow => false
    else false
  Stigi.Test.expect(longSize == 8 && longAlign == 8 && alignedVal == 8 && evalValid, true, "Types layout and AST comptime eval must hold")
