// prompt: law stating that a strictly bounded row count stays within table capacity.
pub lemma def Stigi.Datalog.rowCountWithinCap(rowCount: U64, tableCap: U64) <below(rowCount, tableCap)>: <lessOrEqual(rowCount, tableCap)> =
  @<lessOrEqual(rowCount, tableCap)>
