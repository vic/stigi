// prompt: verify the export names command line has no output directory, one output directory, an extra argument and an empty output argument.
val Stigi.Store.Tests.exportNamesArgsSuite {Stigi.Test}: Stigi.Test.Result =
  val Filled(cmdBuf) = Array.tryFill[U8]{Stigi.Region}(64, 0) else
    case FillFailed => Stigi.Test.expect(false, true, "cmdline buffer alloc")
  val missingBytes: Array[U8](19) = "bin\0--export-names\0"
  Stigi.Array.copy(19, missingBytes, 0, 19, 64, cmdBuf, 0)
  val missing: Bool = Stigi.Store.parseCmdline(64, cmdBuf, 19) is CliInvalid
  val oneDirBytes: Array[U8](24) = "bin\0--export-names\0tree\0"
  Stigi.Array.copy(24, oneDirBytes, 0, 24, 64, cmdBuf, 0)
  val oneDir: Bool = Stigi.Store.parseCmdline(64, cmdBuf, 24) is CliExportNames(19, 23)
  val withOutBytes: Array[U8](28) = "bin\0--export-names\0tree\0out\0"
  Stigi.Array.copy(28, withOutBytes, 0, 28, 64, cmdBuf, 0)
  val withOut: Bool = Stigi.Store.parseCmdline(64, cmdBuf, 28) is CliExportNamesTo(19, 23, 24, 27)
  val extraBytes: Array[U8](30) = "bin\0--export-names\0tree\0out\0x\0"
  Stigi.Array.copy(30, extraBytes, 0, 30, 64, cmdBuf, 0)
  val extra: Bool = Stigi.Store.parseCmdline(64, cmdBuf, 30) is CliInvalid
  val emptyBytes: Array[U8](25) = "bin\0--export-names\0tree\0\0"
  Stigi.Array.copy(25, emptyBytes, 0, 25, 64, cmdBuf, 0)
  val emptyOut: Bool = Stigi.Store.parseCmdline(64, cmdBuf, 25) is CliInvalid
  Stigi.Test.expect(missing && oneDir && withOut && extra && emptyOut, true, "export names accepts one or two directories and rejects none, an extra and an empty argument")
