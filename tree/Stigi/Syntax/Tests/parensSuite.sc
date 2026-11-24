// prompt: verify that a right operand of equal binding strength keeps its parentheses through a parse and print.
val Stigi.Syntax.Tests.parensSuite {Stigi.Test}: Stigi.Test.Result =
  val passed: Bool =
    val Allocated(tables) = Stigi.Syntax.allocate{Stigi.Region}() else
      case AllocationFailed => false
    val text: Array[U8](46) = "// prompt: demo.\nval X: U64 = a - (b - c) - d\n"
    Stigi.Syntax.verifyText(tables, 46, text, 46) is TextMatched(_, _, _)
  Stigi.Test.expect(passed, true, "a right operand of equal binding strength keeps its parentheses")
