// prompt: verify that a negated sum prints with the parentheses of the seed printer.
val Stigi.Syntax.Print.Tests.cycleSuite {Stigi.Test}: Stigi.Test.Result =
  val passed: Bool =
    val Filled(source) = Array.tryFill[U8]{Stigi.Region}(Stigi.Limit.maxUnitBytes, 0) else
      case FillFailed => false
    val Filled(tokens) = Array.tryFill[Stigi.Lex.Token]{Stigi.Region}(Stigi.Limit.maxUnitBytes, Token(TokEnd, 0, 0, 0, 0)) else
      case FillFailed => false
    val Filled(nodes) = Array.tryFill[Stigi.Syntax.Node]{Stigi.Region}(Stigi.Limit.maxUnitNodes, Absent(0)) else
      case FillFailed => false
    val Filled(lists) = Array.tryFill[Stigi.Syntax.NodeAt]{Stigi.Region}(Stigi.Limit.maxUnitNodes, 0) else
      case FillFailed => false
    val Filled(pending) = Array.tryFill[Stigi.Syntax.NodeAt]{Stigi.Region}(Stigi.Limit.maxUnitNodes, 0) else
      case FillFailed => false
    val Filled(output) = Array.tryFill[U8]{Stigi.Region}(Stigi.Limit.maxUnitBytes, 0) else
      case FillFailed => false
    val text: Array[U8](5) = "a + b"
    Stigi.Array.copy[U8](5, text, 0, 5, Stigi.Limit.maxUnitBytes, source, 0)
    nodes(0) := Ref(0, Text(0, 1))
    nodes(1) := Ref(1, Text(4, 5))
    nodes(2) := Binary(2, Plus, 0, 1)
    nodes(3) := Unary(3, Negate, 2)
    val tables: Stigi.Syntax.Tables = Tables(source, tokens, nodes, lists, pending, output)
    val printed: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.printNode(tables, 3, Pen(0), 0, Stigi.Limit.maxParseDepth)
    printed is Pen(8) && output(0) == '-' && output(1) == '(' && output(2) == 'a' && output(4) == '+' && output(6) == 'b' && output(7) == ')'
  Stigi.Test.expect(passed, true, "a negated sum prints as -(a + b)")
