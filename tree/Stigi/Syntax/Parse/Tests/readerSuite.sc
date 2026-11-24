// prompt: verify that a fact group, a type and a pattern read outside the expression cycle print back byte for byte.
val Stigi.Syntax.Parse.Tests.readerSuite {Stigi.Test}: Stigi.Test.Result =
  val passed: Bool =
    val Allocated(tables) = Stigi.Syntax.allocate{Stigi.Region}() else
      case AllocationFailed => false
    val Tables(_, _, _, _, _, output) = tables
    val factText: Array[U8](60) = "<below(i, n), m = i + 1, not (p(1)), below(offset + 1, cap)>"
    val LexDone(_) = Stigi.Syntax.Parse.lexUnit(tables, 60, factText) else
      case LexOverflow | LexSyntaxError(_, _) => false
    val Built(factRoot, _) = Stigi.Syntax.Parse.readFactGroup(tables, Progress(0, 0, 0, 0)) else
      case Failed(_, _) => false
    val factValid: Bool = Stigi.Syntax.Print.printNode(tables, factRoot, Pen(0), 0, Stigi.Limit.maxParseDepth) is Pen(60) && Stigi.Lex.sliceEquals(Stigi.Limit.maxUnitBytes, output, 0, 60, 60, factText)
    val typeText: Array[U8](18) = "Array[U8](4096){r}"
    val LexDone(_) = Stigi.Syntax.Parse.lexUnit(tables, 18, typeText) else
      case LexOverflow | LexSyntaxError(_, _) => false
    val Built(typeRoot, _) = Stigi.Syntax.Parse.readType(tables, Progress(0, 0, 0, 0), Stigi.Limit.maxParseDepth) else
      case Failed(_, _) => false
    val typeValid: Bool = Stigi.Syntax.Print.printNode(tables, typeRoot, Pen(0), 0, Stigi.Limit.maxParseDepth) is Pen(18) && Stigi.Lex.sliceEquals(Stigi.Limit.maxUnitBytes, output, 0, 18, 18, typeText)
    val patternText: Array[U8](35) = "Some(_) | None | 'a'..'z' | (x, -1)"
    val LexDone(_) = Stigi.Syntax.Parse.lexUnit(tables, 35, patternText) else
      case LexOverflow | LexSyntaxError(_, _) => false
    val Built(patternRoot, _) = Stigi.Syntax.Parse.readPattern(tables, Progress(0, 0, 0, 0), Stigi.Limit.maxParseDepth) else
      case Failed(_, _) => false
    val patternValid: Bool = Stigi.Syntax.Print.printNode(tables, patternRoot, Pen(0), 0, Stigi.Limit.maxParseDepth) is Pen(35) && Stigi.Lex.sliceEquals(Stigi.Limit.maxUnitBytes, output, 0, 35, 35, patternText)
    factValid && typeValid && patternValid
  Stigi.Test.expect(passed, true, "a fact group, a type and a pattern read outside the expression cycle print back byte for byte")
