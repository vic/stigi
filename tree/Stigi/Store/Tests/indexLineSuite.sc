// prompt: verify that an index failure of unit 12 named A.d with an exhausted image formats the exact line, chunk by chunk: the index digits, the name, the step words and the reason.
val Stigi.Store.Tests.indexLineSuite {Stigi.Test}: Stigi.Test.Result =
  val code: U64 =
    val Filled(report) = Array.tryFill[U8]{Stigi.Region}(65536, 0) else
      case FillFailed => 99
    val Filled(nameBuf) = Array.tryFill[U8]{Stigi.Region}(Stigi.Limit.maxUnitBytes, 0) else
      case FillFailed => 99
    val copied: U64 = Stigi.Array.copy[U8](3, "A.d", 0, 3, Stigi.Limit.maxUnitBytes, nameBuf, 0)
    val ended: U64 = Stigi.Store.putReasonWord(report, Stigi.Store.putFailedAt{Stigi.Region}(report, 12, nameBuf, 3, IndexStep), ImageExhausted)
    if copied == 3 then
      if Stigi.Store.Tests.chunkIs(report, 0, 22, "names: failed at unit ") then
        if Stigi.Store.Tests.chunkIs(report, 22, 2, "12") then
          if Stigi.Store.Tests.chunkIs(report, 24, 2, " (") && Stigi.Store.Tests.chunkIs(report, 26, 3, "A.d") then
            if Stigi.Store.Tests.chunkIs(report, 29, 16, ") during index: ") then
              if Stigi.Store.Tests.chunkIs(report, 45, 16, "image exhausted\n") then
                if ended == 61 then 0 else 7
              else 6
            else 5
          else 4
        else 3
      else 2
    else 1
  if code == 0 then Stigi.Test.expect(true, true, "the index failure line is exact") else if code == 1 then Stigi.Test.expect(false, true, "clause name copied") else if code == 2 then Stigi.Test.expect(false, true, "clause failure label") else if code == 3 then Stigi.Test.expect(false, true, "clause index digits") else if code == 4 then Stigi.Test.expect(false, true, "clause unit name") else if code == 5 then Stigi.Test.expect(false, true, "clause step words") else if code == 6 then Stigi.Test.expect(false, true, "clause reason words") else if code == 7 then Stigi.Test.expect(false, true, "clause line end") else Stigi.Test.expect(false, true, "clause setup of the index line suite")
