// prompt: verify that a walk failure of unit 7 named A.d with unresolved names formats the exact line, chunk by chunk: the index digits, the name, the step words and the reason.
val Stigi.Store.Tests.walkLineSuite {Stigi.Test}: Stigi.Test.Result =
  val code: U64 =
    val Filled(report) = Array.tryFill[U8]{Stigi.Region}(65536, 0) else
      case FillFailed => 99
    val Filled(nameBuf) = Array.tryFill[U8]{Stigi.Region}(Stigi.Limit.maxUnitBytes, 0) else
      case FillFailed => 99
    val copied: U64 = Stigi.Array.copy[U8](3, "A.d", 0, 3, Stigi.Limit.maxUnitBytes, nameBuf, 0)
    val ended: U64 = Stigi.Store.putReasonWord(report, Stigi.Store.putFailedAt{Stigi.Region}(report, 7, nameBuf, 3, WalkStep), UnresolvedNames)
    if copied == 3 then
      if Stigi.Store.Tests.chunkIs(report, 0, 22, "names: failed at unit ") then
        if Stigi.Store.Tests.chunkIs(report, 22, 1, "7") then
          if Stigi.Store.Tests.chunkIs(report, 23, 2, " (") && Stigi.Store.Tests.chunkIs(report, 25, 3, "A.d") then
            if Stigi.Store.Tests.chunkIs(report, 28, 15, ") during walk: ") then
              if Stigi.Store.Tests.chunkIs(report, 43, 17, "unresolved names\n") then
                if ended == 60 then 0 else 7
              else 6
            else 5
          else 4
        else 3
      else 2
    else 1
  if code == 0 then Stigi.Test.expect(true, true, "the walk failure line is exact") else if code == 1 then Stigi.Test.expect(false, true, "clause name copied") else if code == 2 then Stigi.Test.expect(false, true, "clause failure label") else if code == 3 then Stigi.Test.expect(false, true, "clause index digits") else if code == 4 then Stigi.Test.expect(false, true, "clause unit name") else if code == 5 then Stigi.Test.expect(false, true, "clause step words") else if code == 6 then Stigi.Test.expect(false, true, "clause reason words") else if code == 7 then Stigi.Test.expect(false, true, "clause line end") else Stigi.Test.expect(false, true, "clause setup of the walk line suite")
