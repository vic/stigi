// prompt: verify a facts path that fits has the exact bytes of the directory, a slash, the file name and a null over a buffer of non-null bytes, that a path of maxPathBytes minus one bytes fits, and that one byte more overflows.
val Stigi.Store.Tests.factsPathSuite {Stigi.Test}: Stigi.Test.Result =
  val Filled(pathBuf) = Array.tryFill[U8]{Stigi.Region}(Stigi.Limit.maxPathBytes, 'x') else
    case FillFailed => Stigi.Test.expect(false, true, "path buffer alloc")
  val Filled(longDir) = Array.tryFill[U8]{Stigi.Region}(4085, 'd') else
    case FillFailed => Stigi.Test.expect(false, true, "long dir alloc")
  val shortDir: Array[U8](3) = "out"
  val fits: Bool = Stigi.Store.buildFactsPath(3, shortDir, 0, 3, LaneFile(UnitLane), pathBuf) match
    case PathBuilt(size) => size == 14 && pathBuf(0) == 'o' && pathBuf(2) == 't' && pathBuf(3) == '/' && pathBuf(4) == 'u' && pathBuf(13) == 's' && pathBuf(14) == '\0'
    case PathOverflow => false
  val fitsAtBound: Bool = Stigi.Store.buildFactsPath(4085, longDir, 0, 4084, LaneFile(UnitLane), pathBuf) match
    case PathBuilt(size) => size == 4095 && pathBuf(4083) == 'd' && pathBuf(4084) == '/' && pathBuf(4094) == 's' && pathBuf(4095) == '\0'
    case PathOverflow => false
  val overflows: Bool = Stigi.Store.buildFactsPath(4085, longDir, 0, 4085, LaneFile(UnitLane), pathBuf) is PathOverflow
  Stigi.Test.expect(fits && fitsAtBound && overflows, true, "a fitting facts path has the exact bytes, a path of maxPathBytes minus one bytes fits and one byte more overflows")
