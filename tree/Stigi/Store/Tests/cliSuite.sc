// prompt: verify CLI command parsing, unit path construction, and Stigi.Raw.mixWord determinism.
val Stigi.Store.Tests.cliSuite {Stigi.Test}: Stigi.Test.Result =
  val Filled(cmdBuf) = Array.tryFill[U8]{Stigi.Region}(64, 0) else
    case FillFailed => Stigi.Test.expect(false, true, "cmdline buffer alloc")
  val Filled(pathBuf) = Array.tryFill[U8]{Stigi.Region}(4096, 0) else
    case FillFailed => Stigi.Test.expect(false, true, "path buffer alloc")
  val emptyRes: Bool = Stigi.Store.parseCmdline(64, cmdBuf, 0) is CliRunContracts
  val progBytes: Array[U8](25) = "bin\0--load\0tree\0--emit\0c\0"
  Stigi.Array.copy(25, progBytes, 0, 25, 64, cmdBuf, 0)
  val loadEmitRes: Bool = Stigi.Store.parseCmdline(64, cmdBuf, 25) is CliLoadEmit(11, 15, 23, 24)
  val checkRes: Bool = Stigi.Store.parseCmdline(64, cmdBuf, 16) is CliInvalid
  val verifyBytes: Array[U8](24) = "bin\0--verify-parse\0tree\0"
  Stigi.Array.copy(24, verifyBytes, 0, 24, 64, cmdBuf, 0)
  val verifyRes: Bool = Stigi.Store.parseCmdline(64, cmdBuf, 24) is CliVerifyParse(19, 23)
  val dirName: Array[U8](4) = "tree"
  val unitName: Array[U8](17) = "Stigi.Lex.scanAll"
  val pathRes: Bool = Stigi.Store.buildUnitPath(4, dirName, 0, 4, 17, unitName, 0, 17, pathBuf) match
    case Found(pathLen) => pathLen == 25 && pathBuf(4) == '/' && pathBuf(10) == '/' && pathBuf(25) == '\0'
    case Missing => false
  val mixCheck: Bool = Stigi.Raw.mixWord(0) == 0xe220a8397b1dcdaf
  Stigi.Test.expect(emptyRes && loadEmitRes && checkRes && verifyRes && pathRes && mixCheck, true, "CLI parser, path builder, and mixWord must match expected invariants")
