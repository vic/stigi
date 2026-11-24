// prompt: verify the row size defs equal the bytes the unit, edge and name row writers put for sample rows of the unit A.d.
val Stigi.Store.Tests.rowSizeSuite {Stigi.Test}: Stigi.Test.Result =
  val code: U64 =
    val Filled(report) = Array.tryFill[U8]{Stigi.Region}(65536, 0) else
      case FillFailed => 99
    val Filled(pool) = Array.tryFill[U8]{Stigi.Region}(Stigi.Limit.maxIndexBytes, 0) else
      case FillFailed => 99
    val copied: U64 = Stigi.Array.copy[U8](6, "A.fA.d", 0, 6, Stigi.Limit.maxIndexBytes, pool, 0)
    val unitEnd: U64 = Stigi.Store.putUnitRow(65536, report, 0, Stigi.Limit.maxIndexBytes, pool, 3, 6, DefKind)
    val edgeEnd: U64 = Stigi.Store.putEdgeRow(65536, report, 0, pool, PoolRun(0, 3), PoolRun(3, 6))
    val nameEnd: U64 = Stigi.Store.putNameRow(65536, report, 0, Stigi.Limit.maxIndexBytes, pool, 3, 6)
    val handlerEnd: U64 = Stigi.Store.putUnitRow(65536, report, 0, Stigi.Limit.maxIndexBytes, pool, 3, 6, HandlerKind)
    if copied == 6 then
      if unitEnd == Stigi.Store.unitRowSize(3, 6, DefKind) then
        if edgeEnd == Stigi.Store.edgeRowSize(0, 3, 3, 6) then
          if nameEnd == Stigi.Store.nameRowSize(3, 6) then
            if handlerEnd == Stigi.Store.unitRowSize(3, 6, HandlerKind) then 0 else 5
          else 4
        else 3
      else 2
    else 1
  if code == 0 then Stigi.Test.expect(true, true, "every row size equals the bytes written") else if code == 1 then Stigi.Test.expect(false, true, "pool filled") else if code == 2 then Stigi.Test.expect(false, true, "unit row size") else if code == 3 then Stigi.Test.expect(false, true, "edge row size") else if code == 4 then Stigi.Test.expect(false, true, "name row size") else Stigi.Test.expect(false, true, "handler row size")
