// prompt: verify that units with a local given, a capability, an enum, a multi-head def and an extern def print back byte for byte.
val Stigi.Syntax.Tests.unitSuite {Stigi.Test}: Stigi.Test.Result =
  val passed: Bool =
    val Allocated(tables) = Stigi.Syntax.allocate{Stigi.Region}() else
      case AllocationFailed => false
    val localText: Array[U8](213) = "// prompt: demo.\nval Demo.check {Stigi.NegTest}: Never =\n  given Demo.Bad {Stigi.File}: Stigi.Order[U8]:\n    def lessThan(left: U8, right: U8): Bool = left < right\n  Stigi.NegTest.expect(Stigi.Code.GivenRow, \"x\")\n"
    val localValid: Bool = Stigi.Syntax.verifyText(tables, 213, localText, 213) is TextMatched(_, _, _)
    val capabilityText: Array[U8](152) = "// prompt: demo.\npub capability Demo.File:\n  state Open, Closed\n  start Open\n  stop Closed\n  def close() <at(self, Open)>: Demo.Done <at(self, Closed)>\n"
    val capabilityValid: Bool = Stigi.Syntax.verifyText(tables, 152, capabilityText, 152) is TextMatched(_, _, _)
    val enumText: Array[U8](104) = "// prompt: demo.\nenum Demo.Pair [A: Type]:\n  case Pair(left: A, right: Bool) <below(0, 9)>\n  case Empty\n"
    val enumValid: Bool = Stigi.Syntax.verifyText(tables, 104, enumText, 104) is TextMatched(_, _, _)
    val clauseText: Array[U8](103) = "// prompt: demo.\ndef Demo.pick(kind: U8) <lessOrEqual(kind, 9)>: U64 =\n  case 1 | 2 => 3\n  case _ => 0\n"
    val clauseValid: Bool = Stigi.Syntax.verifyText(tables, 103, clauseText, 103) is TextMatched(_, _, _)
    val externText: Array[U8](77) = "// prompt: demo.\npub extern \"c\" def write(desc: I32) <externStack(512)>: I64\n"
    val externValid: Bool = Stigi.Syntax.verifyText(tables, 77, externText, 77) is TextMatched(_, _, _)
    val broken: Array[U8](33) = "// prompt: demo.\ndef Demo.x( = 1\n"
    val brokenRejected: Bool = Stigi.Syntax.verifyText(tables, 33, broken, 33) is UnitParseFailed(_, _)
    localValid && capabilityValid && enumValid && clauseValid && externValid && brokenRejected
  Stigi.Test.expect(passed, true, "units with a local given, a capability, an enum, a multi-head def and an extern def print back byte for byte, and a broken unit is rejected")
