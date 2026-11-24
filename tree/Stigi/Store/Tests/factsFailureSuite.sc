// prompt: verify the exact failure line bytes of every failure shape and every facts file name.
val Stigi.Store.Tests.factsFailureSuite {Stigi.Test}: Stigi.Test.Result =
  val Filled(report) = Array.tryFill[U8]{Stigi.Region}(65536, 0) else
    case FillFailed => Stigi.Test.expect(false, true, "report filled")
  val full: Bool = Stigi.Store.Tests.failureMatches{Stigi.Region}(report, FactsStopped(LaneFull(EdgeLane)), 44, "names: failed writing edge.facts: lane full\n")
  val edge: Bool = Stigi.Store.Tests.failureMatches{Stigi.Region}(report, FactsStopped(EdgeOutsideImage(123)), 34, "names: edge 123 outside the image\n")
  val long: Bool = Stigi.Store.Tests.failureMatches{Stigi.Region}(report, FileFailed(ModeFile, PathTooLong), 53, "names: failed writing imageMode.facts: path too long\n")
  val failed: Bool = Stigi.Store.Tests.failureMatches{Stigi.Region}(report, FileFailed(LaneFile(UnitLane), WriteFailed), 47, "names: failed writing unit.facts: write failed\n")
  val memory: Bool = Stigi.Store.Tests.failureMatches{Stigi.Region}(report, FactsOutOfMemory, 21, "names: out of memory\n")
  val named: Bool = Stigi.Store.Tests.failureMatches{Stigi.Region}(report, FileFailed(LaneFile(NameLane), WriteFailed), 51, "names: failed writing unitName.facts: write failed\n")
  val under: Bool = Stigi.Store.Tests.failureMatches{Stigi.Region}(report, FactsStopped(LaneFull(StigiLane)), 50, "names: failed writing underStigi.facts: lane full\n")
  val owner: Bool = Stigi.Store.Tests.failureMatches{Stigi.Region}(report, FileFailed(LaneFile(HandlerLane), WriteFailed), 54, "names: failed writing handlerUnit.facts: write failed\n")
  val negative: Bool = Stigi.Store.Tests.failureMatches{Stigi.Region}(report, FileFailed(LaneFile(NegTestLane), PathTooLong), 51, "names: failed writing negtest.facts: path too long\n")
  Stigi.Test.expect(full && edge && long && failed && memory && named && under && owner && negative, true, "every failure line has the exact bytes")
