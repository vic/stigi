// prompt: verify that a slot holding its own index is fetched and a filler slot is a malformed table.
val Stigi.Syntax.Tests.tableSuite {Stigi.Test}: Stigi.Test.Result =
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
    nodes(1) := Name(1, Text(0, 3))
    val tables: Stigi.Syntax.Tables = Tables(source, tokens, nodes, lists, pending, output)
    Stigi.Syntax.readSlot(tables, 1) is Fetched(Name(1, Text(0, 3))) && Stigi.Syntax.readSlot(tables, 2) is MalformedTable(2)
  Stigi.Test.expect(passed, true, "a slot holding its own index is fetched and a filler slot is a malformed table")
