// prompt: verify CLI command parsing, unit path construction, and Stigi.Raw.mixWord determinism.
val Stigi.Store.Tests.cliSuite {Stigi.Test}: Stigi.Test.Result =
  val Filled(cmdBuf) = Array.tryFill[U8]{Stigi.Region}(64, 0) else
    case FillFailed => Stigi.Test.expect(false, true, "cmdline buffer alloc")
  val Filled(pathBuf) = Array.tryFill[U8]{Stigi.Region}(4096, 0) else
    case FillFailed => Stigi.Test.expect(false, true, "path buffer alloc")
  val emptyRes: Bool = Stigi.Store.parseCmdline(64, cmdBuf, 0) match
    case CliRunContracts => true
    case CliCheckTree(_, _) => false
    case CliLoadEmit(_, _, _, _) => false
    case CliInvalid => false
  val progBytes: Array[U8](25) = "bin\0--load\0tree\0--emit\0c\0"
  var idx: U64 = 0
  while idx < 25 do
    @<lessOrEqual(idx, 25)>
    cmdBuf(idx) := progBytes(idx)
    idx = idx + 1
  val loadEmitRes: Bool = Stigi.Store.parseCmdline(64, cmdBuf, 25) match
    case CliLoadEmit(dirBeg, dirFin, outBeg, outFin) => dirBeg == 11 && dirFin == 15 && outBeg == 23 && outFin == 24
    case CliRunContracts => false
    case CliCheckTree(_, _) => false
    case CliInvalid => false
  val dirName: Array[U8](4) = "tree"
  val unitName: Array[U8](17) = "Stigi.Lex.scanAll"
  val pathRes: Bool = Stigi.Store.buildUnitPath(4, dirName, 0, 4, 17, unitName, 0, 17, pathBuf) match
    case Found(pathLen) => pathLen == 25 && pathBuf(4) == 47 && pathBuf(10) == 47 && pathBuf(25) == 0
    case Missing => false
  val mixZero: U64 = Stigi.Raw.mixWord(0)
  val mixCheck: Bool = mixZero == 0xe220a8397b1dcdaf
  val dummySpan: Stigi.Store.UnitSpan = UnitSpan(0, 4, 0, 10, 0, 2)
  val UnitSpan(spanBeg, spanFin, _, _, _, _) = dummySpan
  Stigi.Test.expect(emptyRes && loadEmitRes && pathRes && mixCheck && spanBeg == 0 && spanFin == 4, true, "CLI parser, path builder, and mixWord must match expected invariants")
