// prompt: verify the unit, edge, name and mode rows are formatted with exact bytes, chunk by chunk, for a unit named A.d of kind def, an edge from A.f to A.d, and an enum row.
val Stigi.Store.Tests.rowSuite {Stigi.Test}: Stigi.Test.Result =
  val code: U64 =
    val Filled(report) = Array.tryFill[U8]{Stigi.Region}(65536, 0) else
      case FillFailed => 99
    val Filled(pool) = Array.tryFill[U8]{Stigi.Region}(Stigi.Limit.maxIndexBytes, 0) else
      case FillFailed => 99
    val copied: U64 = Stigi.Array.copy[U8](6, "A.fA.d", 0, 6, Stigi.Limit.maxIndexBytes, pool, 0)
    val unitEnd: U64 = Stigi.Store.putUnitRow(65536, report, 0, Stigi.Limit.maxIndexBytes, pool, 3, 6, DefKind)
    val edgeEnd: U64 = Stigi.Store.putEdgeRow(65536, report, unitEnd, pool, PoolRun(0, 3), PoolRun(3, 6))
    val nameEnd: U64 = Stigi.Store.putNameRow(65536, report, edgeEnd, Stigi.Limit.maxIndexBytes, pool, 3, 6)
    val modeEnd: U64 = Stigi.Store.putModeRow(65536, report, nameEnd)
    val enumEnd: U64 = Stigi.Store.putUnitRow(65536, report, modeEnd, Stigi.Limit.maxIndexBytes, pool, 3, 6, EnumKind)
    if copied == 6 then
      if Stigi.Store.Tests.chunkIs(report, 0, 8, "A.d\tDef\n") then
        if Stigi.Store.Tests.chunkIs(report, 8, 8, "A.f\tA.d\n") then
          if Stigi.Store.Tests.chunkIs(report, 16, 4, "A.d\n") then
            if Stigi.Store.Tests.chunkIs(report, 20, 7, "Kernel\n") then
              if Stigi.Store.Tests.chunkIs(report, 27, 9, "A.d\tEnum\n") then
                if enumEnd == 36 then 0 else 7
              else 6
            else 5
          else 4
        else 3
      else 2
    else 1
  if code == 0 then Stigi.Test.expect(true, true, "every row is formatted with exact bytes") else if code == 1 then Stigi.Test.expect(false, true, "pool filled") else if code == 2 then Stigi.Test.expect(false, true, "unit row bytes") else if code == 3 then Stigi.Test.expect(false, true, "edge row bytes") else if code == 4 then Stigi.Test.expect(false, true, "name row bytes") else if code == 5 then Stigi.Test.expect(false, true, "mode row bytes") else if code == 6 then Stigi.Test.expect(false, true, "enum row bytes") else Stigi.Test.expect(false, true, "end offset")
