// prompt: verify that a block with a val and else arms, a conditional and a match parses and prints back byte for byte.
val Stigi.Syntax.Tests.statementSuite {Stigi.Test}: Stigi.Test.Result =
  val passed: Bool =
    val Allocated(tables) = Stigi.Syntax.allocate{Stigi.Region}() else
      case AllocationFailed => false
    val text: Array[U8](142) = "\n  val Filled(x) = f(1) else\n    case Failed => false\n  if a then\n    b\n  else\n    c\n  x match\n    case Some(y) if y > 1 => y\n    case _ => 0\n"
    val expected: Array[U8](138) = "val Filled(x) = f(1) else\n    case Failed => false\n  if a then\n    b\n  else\n    c\n  x match\n    case Some(y) if y > 1 => y\n    case _ => 0"
    val LexDone(_) = Stigi.Syntax.Parse.lexUnit(tables, 142, text) else
      case LexOverflow | LexSyntaxError(_, _) => false
    val Built(root, _) = Stigi.Syntax.Parse.parseBody(tables, Progress(0, 0, 0, 0), 0, Stigi.Limit.maxParseDepth) else
      case Failed(_, _) => false
    val Tables(_, _, _, _, _, output) = tables
    Stigi.Syntax.Print.printNode(tables, root, Pen(0), 2, Stigi.Limit.maxParseDepth) is Pen(138) && Stigi.Lex.sliceEquals(Stigi.Limit.maxUnitBytes, output, 0, 138, 138, expected)
  Stigi.Test.expect(passed, true, "a block with a val and else arms, a conditional and a match prints back byte for byte")
