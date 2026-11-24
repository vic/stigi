// prompt: verify typeRange and typeWidth path building, failure formatting, exact collected table bytes and wide decimal formatting.
val Stigi.Store.Tests.typesFactsSuite {Stigi.Test}: Stigi.Test.Result =
  val Filled(pathBuf) = Array.tryFill[U8]{Stigi.Region}(Stigi.Limit.maxPathBytes, 0) else
    case FillFailed => Stigi.Test.expect(false, true, "path buffer alloc")
  val Filled(report) = Array.tryFill[U8]{Stigi.Region}(65536, 0) else
    case FillFailed => Stigi.Test.expect(false, true, "report buffer alloc")
  val Filled(rangeBuf) = Array.tryFill[U8]{Stigi.Region}(4096, 0) else
    case FillFailed => Stigi.Test.expect(false, true, "range buffer alloc")
  val Filled(widthBuf) = Array.tryFill[U8]{Stigi.Region}(4096, 0) else
    case FillFailed => Stigi.Test.expect(false, true, "width buffer alloc")
  val outDir: Array[U8](3) = "out"
  val rangePathPassed: Bool = Stigi.Store.buildFactsPath(3, outDir, 0, 3, TypeRangeFile, pathBuf) match
    case PathBuilt(size) => size == 19 && Stigi.Lex.sliceEquals(Stigi.Limit.maxPathBytes, pathBuf, 0, 19, 19, "out/typeRange.facts") && pathBuf(19) == '\0'
    case PathOverflow => false
  val widthPathPassed: Bool = Stigi.Store.buildFactsPath(3, outDir, 0, 3, TypeWidthFile, pathBuf) match
    case PathBuilt(size) => size == 19 && Stigi.Lex.sliceEquals(Stigi.Limit.maxPathBytes, pathBuf, 0, 19, 19, "out/typeWidth.facts") && pathBuf(19) == '\0'
    case PathOverflow => false
  val writeFailLen: U64 = Stigi.Store.putExportFailure{Stigi.Region}(report, 0, TypesExport, FileFailed(TypeRangeFile, WriteFailed))
  val writeFailPassed: Bool = Stigi.Lex.sliceEquals(65536, report, 0, writeFailLen, 52, "types: failed writing typeRange.facts: write failed\n")
  val longFailLen: U64 = Stigi.Store.putExportFailure{Stigi.Region}(report, 0, TypesExport, FileFailed(TypeWidthFile, PathTooLong))
  val longFailPassed: Bool = Stigi.Lex.sliceEquals(65536, report, 0, longFailLen, 53, "types: failed writing typeWidth.facts: path too long\n")
  val TypeCursors(rangeLen, widthLen) = Stigi.Store.collectTypeTables{Stigi.Region}(rangeBuf, widthBuf, 0, 0)
  val expectedRange: Array[U8](125) = "I32\t-2147483648\t2147483647\nI64\t-9223372036854775808\t9223372036854775807\nU32\t0\t4294967295\nU64\t0\t18446744073709551615\nU8\t0\t255\n"
  val expectedWidth: Array[U8](33) = "I32\t32\nI64\t64\nU32\t32\nU64\t64\nU8\t8\n"
  val tablesPassed: Bool = rangeLen == 125 && widthLen == 33 && Stigi.Lex.sliceEquals(4096, rangeBuf, 0, rangeLen, 125, expectedRange) && Stigi.Lex.sliceEquals(4096, widthBuf, 0, widthLen, 33, expectedWidth)
  val posWideEnd: U64 = Stigi.Store.putWideNumber{Stigi.Region}(65536, report, 0, WideInt(0, 1))
  val posWidePassed: Bool = Stigi.Lex.sliceEquals(65536, report, 0, posWideEnd, 20, "18446744073709551616")
  val minWideEnd: U64 = Stigi.Store.putWideNumber{Stigi.Region}(65536, report, 0, WideInt(0, I64.ofU64(9223372036854775808)))
  val minWidePassed: Bool = Stigi.Lex.sliceEquals(65536, report, 0, minWideEnd, 40, "-170141183460469231731687303715884105728")
  Stigi.Test.expect(rangePathPassed && widthPathPassed && writeFailPassed && longFailPassed && tablesPassed && posWidePassed && minWidePassed, true, "type tables, paths, failures and wide decimal formatting must match exact bytes")
