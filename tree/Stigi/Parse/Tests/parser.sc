// prompt: verify parsing traits, piped expressions, tests, and canonical header printing via Writer.
val Stigi.Parse.Tests.parser {Stigi.Test}: Stigi.Test.Result =
  Stigi.Parse.Laws.precedenceBound(TokPipe, 6)
  val defSrc: Array[U8](55) = "// prompt: demo.\npub def Demo.calc(x: U64) = a + b |> f"
  val traitSrc: Array[U8](43) = "// prompt: eq.\npub trait Demo.Eq [A: Type]:"
  val testSrc: Array[U8](49) = "// prompt: t.\nval Demo.Tests.one {Stigi.Test} = 1"
  val parseValid: Bool =
    val Filled(toks) = Array.tryFill[Stigi.Lex.Token]{Stigi.Region}(64, Token(TokEnd, 0, 0, 0, 0)) else
      case FillFailed => false
    val Filled(nodes) = Array.tryFill[Stigi.Ast.Node]{Stigi.Region}(64, Node(ExprUnit, 0, 0, 0, 0, 0)) else
      case FillFailed => false
    val LexDone(tokCount) = Stigi.Lex.scanAll(55, defSrc, 55, 64, toks) else
      case LexOverflow => false
      case LexSyntaxError(_, _) => false
    val ParsedUnit(header, nodeCount) = Stigi.Parse.parseUnit(55, defSrc, 64, toks, tokCount, 64, nodes) else
      case ParseFailed(_) => false
    val UnitHeader(defKind, defPub, _, _, _, _, rootIdx) = header
    val rootValid: Bool =
      if rootIdx < 64 && nodeCount >= 4 then
        val Node(rootKind, _, _, _, _, _) = nodes(rootIdx)
        defKind == DeclDef && defPub && rootKind == ExprPipe
      else false
    val hdrMatch: Bool = Stigi.Parse.Tests.checkHeaderPrint{Stigi.Region}(55, defSrc, 55, header)
    val LexDone(traitTokCount) = Stigi.Lex.scanAll(43, traitSrc, 43, 64, toks) else
      case LexOverflow => false
      case LexSyntaxError(_, _) => false
    val ParsedUnit(traitHeader, _) = Stigi.Parse.parseUnit(43, traitSrc, 64, toks, traitTokCount, 64, nodes) else
      case ParseFailed(_) => false
    val UnitHeader(traitKind, _, _, _, _, _, _) = traitHeader
    val LexDone(suiteTokCount) = Stigi.Lex.scanAll(49, testSrc, 49, 64, toks) else
      case LexOverflow => false
      case LexSyntaxError(_, _) => false
    val ParsedUnit(suiteHeader, _) = Stigi.Parse.parseUnit(49, testSrc, 64, toks, suiteTokCount, 64, nodes) else
      case ParseFailed(_) => false
    val UnitHeader(suiteKind, _, _, _, _, _, _) = suiteHeader
    rootValid && hdrMatch && traitKind == DeclTrait && suiteKind == DeclTest
  Stigi.Test.expect(parseValid, true, "L0 parser must parse defs with pipes, traits, tests, and print canonical headers")
