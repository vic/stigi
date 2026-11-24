// prompt: whether a failure line is written with exactly the bytes of a literal line.
def Stigi.Store.Tests.failureMatches {region: Stigi.Region, Stigi.Mut} (report: Array[U8](65536), failure: Stigi.Store.FactsFailure, litLen: U64, lit: Array[U8](litLen)) <below(litLen, Stigi.Limit.maxSourceBytes)>: Bool =
  val end: U64 = Stigi.Store.putFactsFailure{region}(report, 0, failure)
  Stigi.Lex.sliceEquals(65536, report, 0, end, litLen, lit)
