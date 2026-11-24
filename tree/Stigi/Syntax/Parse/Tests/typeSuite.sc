// prompt: verify that a type with type, length and capability arguments parses into a postorder node table.
val Stigi.Syntax.Parse.Tests.typeSuite {Stigi.Test}: Stigi.Test.Result =
  val passed: Bool =
    val Allocated(tables) = Stigi.Syntax.allocate{Stigi.Region}() else
      case AllocationFailed => false
    val text: Array[U8](18) = "Array[U8](4096){r}"
    val LexDone(_) = Stigi.Syntax.Parse.lexUnit(tables, 18, text) else
      case LexOverflow | LexSyntaxError(_, _) => false
    val Built(root, _) = Stigi.Syntax.Parse.readType(tables, Progress(0, 0, 0, 0), Stigi.Limit.maxParseDepth) else
      case Failed(_, _) => false
    root == 4 && Stigi.Syntax.readSlot(tables, 4) is Fetched(TypeRef(4, Text(0, 5), Span(0, 1), Span(1, 2), 3))
  Stigi.Test.expect(passed, true, "a type with type, length and capability arguments parses into a postorder node table")
