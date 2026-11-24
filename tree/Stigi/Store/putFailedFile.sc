// prompt: write the start of a failed file line, the export prefix, the words and the file name, giving the new end.
def Stigi.Store.putFailedFile {Stigi.Mut} (report: Array[U8](65536), at: U64, kind: Stigi.Store.ExportKind, file: Stigi.Store.FactsFile) <lessOrEqual(at, 65536)>: U64 <lessOrEqual(result, 65536)> =
  val prefixed: U64 = Stigi.Store.putExportPrefix(report, at, kind)
  val head: U64 = Stigi.Array.copy[U8](15, "failed writing ", 0, 15, 65536, report, prefixed)
  Stigi.Store.putFileName(65536, report, head, file)
