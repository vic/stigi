// prompt: verify that crossSame is true only when both byte runs of the two pools have the same length and the same bytes: equal runs, a longer right run, a longer left run, a differing last byte and two empty runs.
val Stigi.Names.Tests.crossSameSuite {Stigi.Test}: Stigi.Test.Result =
  val code: U64 =
    val Filled(left) = Array.tryFill[U8]{Stigi.Region}(Stigi.Limit.maxIndexBytes, 0) else
      case FillFailed => 99
    val Filled(right) = Array.tryFill[U8]{Stigi.Region}(Stigi.Limit.maxIndexBytes, 0) else
      case FillFailed => 99
    val Filled(other) = Array.tryFill[U8]{Stigi.Region}(Stigi.Limit.maxIndexBytes, 0) else
      case FillFailed => 99
    val textSame: Array[U8](4) = "abcd"
    val textOther: Array[U8](4) = "abce"
    val copiedLeft: U64 = Stigi.Array.copy[U8](4, textSame, 0, 4, Stigi.Limit.maxIndexBytes, left, 0)
    val copiedRight: U64 = Stigi.Array.copy[U8](4, textSame, 0, 4, Stigi.Limit.maxIndexBytes, right, 0)
    val copiedOther: U64 = Stigi.Array.copy[U8](4, textOther, 0, 4, Stigi.Limit.maxIndexBytes, other, 0)
    if copiedLeft == 4 then
      if copiedRight == 4 then
        if copiedOther == 4 then
          if Stigi.Names.crossSame(left, 0, 4, right, 0, 4) then
            if Stigi.Names.crossSame(left, 0, 3, right, 0, 4) then 5 else if Stigi.Names.crossSame(left, 0, 4, right, 0, 3) then 6 else if Stigi.Names.crossSame(left, 0, 4, other, 0, 4) then 7 else if Stigi.Names.crossSame(left, 0, 0, right, 0, 0) then 0 else 8
          else 4
        else 3
      else 2
    else 1
  if code == 0 then Stigi.Test.expect(true, true, "crossSame holds for equal runs and for two empty runs and fails for unequal runs") else if code == 4 then Stigi.Test.expect(false, true, "clause equal runs are the same") else if code == 5 then Stigi.Test.expect(false, true, "clause a longer right run is not the same") else if code == 6 then Stigi.Test.expect(false, true, "clause a longer left run is not the same") else if code == 7 then Stigi.Test.expect(false, true, "clause a differing last byte is not the same") else if code == 8 then Stigi.Test.expect(false, true, "clause two empty runs are the same") else Stigi.Test.expect(false, true, "clause setup of the crossSame suite")
