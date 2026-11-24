// prompt: verify that texts after Stigi.Code and Stigi.Id name builtins only when the rest is a known name, and the bare prefixes do not.
val Stigi.Names.Tests.builtinPrefixSuite {Stigi.Test}: Stigi.Test.Result =
  val passed: Bool =
    val Allocated(tables) = Stigi.Syntax.allocate{Stigi.Region}() else
      case AllocationFailed => false
    val probe: Array[U8](127) = "// prompt: demo.\ndef A.B.c(x: U64): U64 =\n  Stigi.Code.IndexUnproved + Stigi.Code.Nonsense + Stigi.Id.Unit + Stigi.Id.Nonsense\n"
    val Parsed(_, _, _) = Stigi.Syntax.Parse.parseUnit(tables, 127, probe, 127) else
      case ParseRejected(_, _) | LexRejected(_) => false
    val named: Bool = Stigi.Names.isBuiltin(tables, 44, 68, 0, 1172) && Stigi.Names.isBuiltin(tables, 93, 106, 0, 1172)
    val unnamed: Bool = Stigi.Names.isBuiltin(tables, 71, 90, 0, 1172) == false && Stigi.Names.isBuiltin(tables, 109, 126, 0, 1172) == false
    val bare: Bool = Stigi.Names.isBuiltin(tables, 44, 55, 0, 1172) == false && Stigi.Names.isBuiltin(tables, 93, 102, 0, 1172) == false
    named && unnamed && bare
  Stigi.Test.expect(passed, true, "a rest after Stigi.Code or Stigi.Id is a builtin only when it is a known name")
