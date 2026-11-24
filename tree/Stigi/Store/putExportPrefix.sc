// prompt: write the command prefix word for a names or types export report line, giving the new end.
def Stigi.Store.putExportPrefix {Stigi.Mut} (report: Array[U8](65536), at: U64, kind: Stigi.Store.ExportKind) <lessOrEqual(at, 65536)>: U64 <lessOrEqual(result, 65536)> =
  kind match
    case NamesExport => Stigi.Array.copy[U8](7, "names: ", 0, 7, 65536, report, at)
    case TypesExport => Stigi.Array.copy[U8](7, "types: ", 0, 7, 65536, report, at)
