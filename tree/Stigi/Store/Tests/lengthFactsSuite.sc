// prompt: verify lengthTerm, linearForm, termOffset, and sameLength path building, failure lines, row formatting, and parsed unit collection.
val Stigi.Store.Tests.lengthFactsSuite {Stigi.Test}: Stigi.Test.Result =
  val Filled(pathBuf) = Array.tryFill[U8]{Stigi.Region}(Stigi.Limit.maxPathBytes, 0) else
    case FillFailed => Stigi.Test.expect(false, true, "path alloc")
  val Filled(report) = Array.tryFill[U8]{Stigi.Region}(65536, 0) else
    case FillFailed => Stigi.Test.expect(false, true, "report alloc")
  val Filled(pool) = Array.tryFill[U8]{Stigi.Region}(Stigi.Limit.maxIndexBytes, 0) else
    case FillFailed => Stigi.Test.expect(false, true, "pool alloc")
  val Filled(rowBuf) = Array.tryFill[U8]{Stigi.Region}(256, 0) else
    case FillFailed => Stigi.Test.expect(false, true, "row buf alloc")
  val outDir: Array[U8](3) = "out"
  val pathLen: Bool = Stigi.Store.buildFactsPath(3, outDir, 0, 3, LengthTermFile, pathBuf) match
    case PathBuilt(builtLen) => builtLen == 20 && Stigi.Lex.sliceEquals(Stigi.Limit.maxPathBytes, pathBuf, 0, 20, 20, "out/lengthTerm.facts")
    case PathOverflow => false
  val pathLin: Bool = Stigi.Store.buildFactsPath(3, outDir, 0, 3, LinearFormFile, pathBuf) match
    case PathBuilt(builtLen) => builtLen == 20 && Stigi.Lex.sliceEquals(Stigi.Limit.maxPathBytes, pathBuf, 0, 20, 20, "out/linearForm.facts")
    case PathOverflow => false
  val pathOff: Bool = Stigi.Store.buildFactsPath(3, outDir, 0, 3, TermOffsetFile, pathBuf) match
    case PathBuilt(builtLen) => builtLen == 20 && Stigi.Lex.sliceEquals(Stigi.Limit.maxPathBytes, pathBuf, 0, 20, 20, "out/termOffset.facts")
    case PathOverflow => false
  val pathSame: Bool = Stigi.Store.buildFactsPath(3, outDir, 0, 3, SameLengthFile, pathBuf) match
    case PathBuilt(builtLen) => builtLen == 20 && Stigi.Lex.sliceEquals(Stigi.Limit.maxPathBytes, pathBuf, 0, 20, 20, "out/sameLength.facts")
    case PathOverflow => false
  val failLen: U64 = Stigi.Store.putExportFailure{Stigi.Region}(report, 0, TypesExport, FactsStopped(TypeLaneFull(LengthTermFile)))
  val failValid: Bool = Stigi.Lex.sliceEquals(65536, report, 0, failLen, 50, "types: failed writing lengthTerm.facts: lane full\n")
  Stigi.Array.copy(4, "Demo", 0, 4, Stigi.Limit.maxIndexBytes, pool, 0)
  val unitSlice: Stigi.Store.NameSlice = NameSlice(pool, 0, 4)
  val atomEnd: U64 = Stigi.Store.putLengthAtomRow{Stigi.Region}(256, rowBuf, 0, unitSlice, 0)
  val atomValid: Bool = Stigi.Lex.sliceEquals(256, rowBuf, 0, atomEnd, 8, "Demo:l0\n")
  val sameEnd: U64 = Stigi.Store.putSameLengthRow{Stigi.Region}(256, rowBuf, 0, unitSlice, LengthPair(0, 1))
  val sameValid: Bool = Stigi.Lex.sliceEquals(256, rowBuf, 0, sameEnd, 16, "Demo:l0\tDemo:l1\n")
  val offEnd: U64 = Stigi.Store.putTermOffsetRow{Stigi.Region}(256, rowBuf, 0, unitSlice, LengthPair(2, 0), 1)
  val offValid: Bool = Stigi.Lex.sliceEquals(256, rowBuf, 0, offEnd, 18, "Demo:l2\tDemo:l0\t1\n")
  val Sampled(fakeImage, _) = Stigi.Names.Tests.threeUnits{Stigi.Region}(1, 7) else
    case SampleUnavailable => Stigi.Test.expect(false, true, "sample image")
  val Allocated(synTables) = Stigi.Syntax.allocate{Stigi.Region}() else
    case AllocationFailed => Stigi.Test.expect(false, true, "syn alloc")
  val LengthScratchReady(scratch) = Stigi.Store.allocateLengthScratch{Stigi.Region}(512, synTables) else
    case LengthScratchUnavailable => Stigi.Test.expect(false, true, "length scratch alloc")
  val LengthLanesReady(lanes) = Stigi.Store.allocateLengthLanes{Stigi.Region}(4096) else
    case LengthLanesUnavailable => Stigi.Test.expect(false, true, "length lanes alloc")
  val srcGood: Array[U8](135) = "// prompt: demo.\ndef Demo.pair(n: U64, firstBuf: Array[U8](n + 1), secondBuf: Array[U8](1 + n), thirdBuf: Array[U8](n + 2)): U64 =\n  n\n"
  val srcBad: Array[U8](95) = "// prompt: demo.\ndef Demo.bad(rows: U64, cols: U64, buf: Array[U8](rows * cols)): U64 =\n  rows\n"
  val srcTwo: Array[U8](112) = "// prompt: demo.\ndef Demo.two(n: U64, k: U64, leftBuf: Array[U8](n + k), rightBuf: Array[U8](k + n)): U64 =\n  n\n"
  val srcMissing: Array[U8](77) = "// prompt: demo.\ndef Demo.missing(buf: Array[U8](missingVar + 1)): U64 =\n  0\n"
  val goodCollected: Bool = Stigi.Store.collectParsedUnitLengths{Stigi.Region}(scratch, lanes, fakeImage, 0, 135, srcGood) match
    case LengthsStopped(_) => false
    case LengthsCollected(LengthLanes(Lane(lenCap, lenBytes, lenEnd), Lane(linCap, linBytes, linEnd), Lane(offCap, offBytes, offEndBytes), Lane(sameCap, sameBytes, sameEndBytes), rowTotal)) =>
      val expLen: Array[U8](21) = "A.d:l0\nA.d:l1\nA.d:l2\n"
      val expOff: Array[U8](32) = "A.d:l2\tA.d:l0\t1\nA.d:l2\tA.d:l1\t1\n"
      val expSame: Array[U8](28) = "A.d:l0\tA.d:l1\nA.d:l1\tA.d:l0\n"
      rowTotal == 10 && lenEnd == 21 && linEnd == 21 && offEndBytes == 32 && sameEndBytes == 28 && Stigi.Lex.sliceEquals(lenCap, lenBytes, 0, lenEnd, 21, expLen) && Stigi.Lex.sliceEquals(linCap, linBytes, 0, linEnd, 21, expLen) && Stigi.Lex.sliceEquals(offCap, offBytes, 0, offEndBytes, 32, expOff) && Stigi.Lex.sliceEquals(sameCap, sameBytes, 0, sameEndBytes, 28, expSame)
  val LengthLanesReady(badLanes) = Stigi.Store.allocateLengthLanes{Stigi.Region}(4096) else
    case LengthLanesUnavailable => Stigi.Test.expect(false, true, "bad lanes alloc")
  val badCollected: Bool = Stigi.Store.collectParsedUnitLengths{Stigi.Region}(scratch, badLanes, fakeImage, 0, 95, srcBad) match
    case LengthsStopped(_) => false
    case LengthsCollected(LengthLanes(Lane(lenCap, lenBytes, lenEnd), Lane(_, _, linEnd), Lane(_, _, offEndBytes), Lane(_, _, sameEndBytes), rowTotal)) =>
      rowTotal == 1 && lenEnd == 7 && linEnd == 0 && offEndBytes == 0 && sameEndBytes == 0 && Stigi.Lex.sliceEquals(lenCap, lenBytes, 0, lenEnd, 7, "A.d:l0\n")
  val LengthLanesReady(twoLanes) = Stigi.Store.allocateLengthLanes{Stigi.Region}(4096) else
    case LengthLanesUnavailable => Stigi.Test.expect(false, true, "two lanes alloc")
  val twoCollected: Bool = Stigi.Store.collectParsedUnitLengths{Stigi.Region}(scratch, twoLanes, fakeImage, 0, 112, srcTwo) match
    case LengthsStopped(_) => false
    case LengthsCollected(LengthLanes(Lane(lenCap, lenBytes, lenEnd), Lane(linCap, linBytes, linEnd), Lane(_, _, offEndBytes), Lane(sameCap, sameBytes, sameEndBytes), rowTotal)) =>
      val expTwoLen: Array[U8](14) = "A.d:l0\nA.d:l1\n"
      val expTwoSame: Array[U8](28) = "A.d:l0\tA.d:l1\nA.d:l1\tA.d:l0\n"
      rowTotal == 6 && lenEnd == 14 && linEnd == 14 && offEndBytes == 0 && sameEndBytes == 28 && Stigi.Lex.sliceEquals(lenCap, lenBytes, 0, lenEnd, 14, expTwoLen) && Stigi.Lex.sliceEquals(linCap, linBytes, 0, linEnd, 14, expTwoLen) && Stigi.Lex.sliceEquals(sameCap, sameBytes, 0, sameEndBytes, 28, expTwoSame)
  val LengthLanesReady(missLanes) = Stigi.Store.allocateLengthLanes{Stigi.Region}(4096) else
    case LengthLanesUnavailable => Stigi.Test.expect(false, true, "miss lanes alloc")
  val missCollected: Bool = Stigi.Store.collectParsedUnitLengths{Stigi.Region}(scratch, missLanes, fakeImage, 0, 77, srcMissing) match
    case LengthsStopped(_) => false
    case LengthsCollected(LengthLanes(Lane(lenCap, lenBytes, lenEnd), Lane(_, _, linEnd), Lane(_, _, offEndBytes), Lane(_, _, sameEndBytes), rowTotal)) =>
      rowTotal == 1 && lenEnd == 7 && linEnd == 0 && offEndBytes == 0 && sameEndBytes == 0 && Stigi.Lex.sliceEquals(lenCap, lenBytes, 0, lenEnd, 7, "A.d:l0\n")
  Stigi.Test.expect(pathLen && pathLin && pathOff && pathSame && failValid && atomValid && sameValid && offValid && goodCollected && badCollected && twoCollected && missCollected, true, "length fact files, rows, and parsed unit collection must match exact bytes")
