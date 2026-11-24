// prompt: buffers one parse check reuses for every unit: path, index text, unit source, report text and the syntax tables.
enum Stigi.Store.ParseScratch:
  case ParseScratch(path: Array[U8](4096), index: Array[U8](65536), unit: Array[U8](65536), report: Array[U8](65536), tables: Stigi.Syntax.Tables)
