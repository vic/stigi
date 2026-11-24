// prompt: verify a guarded sample that does not fit the expression or arm tables overflows instead of being rejected as invalid.
val Stigi.Elaborate.Tests.guardCapacity {Stigi.Test}: Stigi.Test.Result =
  Stigi.Test.expect(Stigi.Elaborate.Tests.overflowsAt{Stigi.Region}(16, 8), false, "the sample must fit sixteen expressions and eight arms")
  Stigi.Test.expect(Stigi.Elaborate.Tests.overflowsAt{Stigi.Region}(7, 8), true, "seven expressions must overflow at the full select")
  Stigi.Test.expect(Stigi.Elaborate.Tests.overflowsAt{Stigi.Region}(6, 8), true, "six expressions must overflow at the choice node")
  Stigi.Test.expect(Stigi.Elaborate.Tests.overflowsAt{Stigi.Region}(5, 8), true, "five expressions must overflow at the tail select")
  Stigi.Test.expect(Stigi.Elaborate.Tests.overflowsAt{Stigi.Region}(3, 8), true, "three expressions must overflow")
  Stigi.Test.expect(Stigi.Elaborate.Tests.overflowsAt{Stigi.Region}(16, 4), true, "four arms must overflow")
  Stigi.Test.expect(Stigi.Elaborate.Tests.overflowsAt{Stigi.Region}(16, 3), true, "three arms must overflow")
