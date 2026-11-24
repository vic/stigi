// prompt: write the start of a failed file line, the words and the file name, giving the new end.
def Stigi.Store.putFailedFile {Stigi.Mut} (report: Array[U8](65536), at: U64, file: Stigi.Store.FactsFile) <lessOrEqual(at, 65536)>: U64 <lessOrEqual(result, 65536)> =
  val head: U64 = Stigi.Array.copy[U8](22, "names: failed writing ", 0, 22, 65536, report, at)
  Stigi.Store.putFileName(65536, report, head, file)
