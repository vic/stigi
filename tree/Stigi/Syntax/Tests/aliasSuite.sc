// prompt: verify that alias lines print back byte for byte and that an alias line without its equal sign is rejected.
val Stigi.Syntax.Tests.aliasSuite {Stigi.Test}: Stigi.Test.Result =
  val passed: Bool =
    val Allocated(tables) = Stigi.Syntax.allocate{Stigi.Region}() else
      case AllocationFailed => false
    val aliasText: Array[U8](113) = "// prompt: demo.\nalias Cap = Stigi.Clock\nalias Fmt = Demo.Format.Long\ndef Demo.tick(x: U64): U64 =\n  Fmt.size(x)\n"
    val aliasValid: Bool = Stigi.Syntax.verifyText(tables, 113, aliasText, 113) is TextMatched(_, _, _)
    val brokenText: Array[U8](72) = "// prompt: demo.\nalias Cap Stigi.Clock\ndef Demo.tick(x: U64): U64 =\n  x\n"
    val brokenRejected: Bool = Stigi.Syntax.verifyText(tables, 72, brokenText, 72) is UnitParseFailed(_, _)
    aliasValid && brokenRejected
  Stigi.Test.expect(passed, true, "alias lines print back byte for byte and an alias line without its equal sign is rejected")
