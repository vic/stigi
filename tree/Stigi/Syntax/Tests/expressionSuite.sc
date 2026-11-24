// prompt: verify that an expression with calls, operators, an is test and an assertion parses and prints back byte for byte.
val Stigi.Syntax.Tests.expressionSuite {Stigi.Test}: Stigi.Test.Result =
  val passed: Bool =
    val Allocated(tables) = Stigi.Syntax.allocate{Stigi.Region}() else
      case AllocationFailed => false
    val text: Array[U8](75) = "f[U8]{r}(a + b * (c - d), -x, y is Some(_) | None)@<below(i, n), m = i + 1>"
    val LexDone(_) = Stigi.Syntax.Parse.lexUnit(tables, 75, text) else
      case LexOverflow | LexSyntaxError(_, _) => false
    val Built(root, _) = Stigi.Syntax.Parse.parseExpr(tables, Progress(0, 0, 0, 0), 0, 0, Stigi.Limit.maxParseDepth) else
      case Failed(_, _) => false
    val Tables(_, _, _, _, _, output) = tables
    Stigi.Syntax.Print.printNode(tables, root, Pen(0), 0, Stigi.Limit.maxParseDepth) is Pen(75) && Stigi.Lex.sliceEquals(Stigi.Limit.maxUnitBytes, output, 0, 75, 75, text)
  Stigi.Test.expect(passed, true, "an expression with calls, operators, an is test and an assertion prints back byte for byte")
