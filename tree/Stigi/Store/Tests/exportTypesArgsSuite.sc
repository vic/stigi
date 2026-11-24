// prompt: verify the export types command line requires two non-empty directory arguments and rejects missing, single, extra or empty arguments.
val Stigi.Store.Tests.exportTypesArgsSuite {Stigi.Test}: Stigi.Test.Result =
  val Filled(cmdBuf) = Array.tryFill[U8]{Stigi.Region}(64, 0) else
    case FillFailed => Stigi.Test.expect(false, true, "cmdline buffer alloc")
  val missingBytes: Array[U8](19) = "bin\0--export-types\0"
  Stigi.Array.copy(19, missingBytes, 0, 19, 64, cmdBuf, 0)
  val missing: Bool = Stigi.Store.parseCmdline(64, cmdBuf, 19) is CliInvalid
  val oneDirBytes: Array[U8](24) = "bin\0--export-types\0tree\0"
  Stigi.Array.copy(24, oneDirBytes, 0, 24, 64, cmdBuf, 0)
  val oneDir: Bool = Stigi.Store.parseCmdline(64, cmdBuf, 24) is CliInvalid
  val withOutBytes: Array[U8](28) = "bin\0--export-types\0tree\0out\0"
  Stigi.Array.copy(28, withOutBytes, 0, 28, 64, cmdBuf, 0)
  val withOut: Bool = Stigi.Store.parseCmdline(64, cmdBuf, 28) is CliExportTypes(19, 23, 24, 27)
  val extraBytes: Array[U8](30) = "bin\0--export-types\0tree\0out\0x\0"
  Stigi.Array.copy(30, extraBytes, 0, 30, 64, cmdBuf, 0)
  val extra: Bool = Stigi.Store.parseCmdline(64, cmdBuf, 30) is CliInvalid
  val emptyOutBytes: Array[U8](25) = "bin\0--export-types\0tree\0\0"
  Stigi.Array.copy(25, emptyOutBytes, 0, 25, 64, cmdBuf, 0)
  val emptyOut: Bool = Stigi.Store.parseCmdline(64, cmdBuf, 25) is CliInvalid
  val emptyTreeBytes: Array[U8](24) = "bin\0--export-types\0\0out\0"
  Stigi.Array.copy(24, emptyTreeBytes, 0, 24, 64, cmdBuf, 0)
  val emptyTree: Bool = Stigi.Store.parseCmdline(64, cmdBuf, 24) is CliInvalid
  Stigi.Test.expect(missing && oneDir && withOut && extra && emptyOut && emptyTree, true, "export types must require two non-empty arguments")
