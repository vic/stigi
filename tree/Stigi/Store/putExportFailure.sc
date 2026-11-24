// prompt: write the single failure line of a names or types export into the report buffer at a position, giving the new end.
def Stigi.Store.putExportFailure {region: Stigi.Region, Stigi.Mut} (report: Array[U8](65536), at: U64, kind: Stigi.Store.ExportKind, failure: Stigi.Store.FactsFailure) <lessOrEqual(at, 65536)>: U64 <lessOrEqual(result, 65536)> =
  failure match
    case FactsStopped(LaneFull(lane)) =>
      val named: U64 = Stigi.Store.putFailedFile(report, at, kind, LaneFile(lane))
      Stigi.Array.copy[U8](12, ": lane full\n", 0, 12, 65536, report, named)
    case FactsStopped(TypeLaneFull(file)) =>
      val named: U64 = Stigi.Store.putFailedFile(report, at, kind, file)
      Stigi.Array.copy[U8](12, ": lane full\n", 0, 12, 65536, report, named)
    case FactsStopped(EdgeOutsideImage(edge)) =>
      val prefixed: U64 = Stigi.Store.putExportPrefix(report, at, kind)
      val head: U64 = Stigi.Array.copy[U8](5, "edge ", 0, 5, 65536, report, prefixed)
      val counted: U64 = Stigi.Store.putNumber{region}(report, head, edge)
      Stigi.Array.copy[U8](19, " outside the image\n", 0, 19, 65536, report, counted)
    case FileFailed(file, PathTooLong) =>
      val named: U64 = Stigi.Store.putFailedFile(report, at, kind, file)
      Stigi.Array.copy[U8](16, ": path too long\n", 0, 16, 65536, report, named)
    case FileFailed(file, WriteFailed) =>
      val named: U64 = Stigi.Store.putFailedFile(report, at, kind, file)
      Stigi.Array.copy[U8](15, ": write failed\n", 0, 15, 65536, report, named)
    case FactsOutOfMemory =>
      val prefixed: U64 = Stigi.Store.putExportPrefix(report, at, kind)
      Stigi.Array.copy[U8](14, "out of memory\n", 0, 14, 65536, report, prefixed)
