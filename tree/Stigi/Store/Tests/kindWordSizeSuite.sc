// prompt: verify the kind word size equals the bytes the kind word writer puts for all thirteen unit kinds.
val Stigi.Store.Tests.kindWordSizeSuite {Stigi.Test}: Stigi.Test.Result =
  val Filled(report) = Array.tryFill[U8]{Stigi.Region}(65536, 0) else
    case FillFailed => Stigi.Test.expect(false, true, "report filled")
  val agrees: Bool = Stigi.Store.Tests.kindWordAgrees(report, 0, DefKind) && Stigi.Store.Tests.kindWordAgrees(report, 0, ValKind) && Stigi.Store.Tests.kindWordAgrees(report, 0, EnumKind) && Stigi.Store.Tests.kindWordAgrees(report, 0, TypeKind) && Stigi.Store.Tests.kindWordAgrees(report, 0, CapabilityKind) && Stigi.Store.Tests.kindWordAgrees(report, 0, HandlerKind) && Stigi.Store.Tests.kindWordAgrees(report, 0, TraitKind) && Stigi.Store.Tests.kindWordAgrees(report, 0, GivenKind) && Stigi.Store.Tests.kindWordAgrees(report, 0, ExternKind) && Stigi.Store.Tests.kindWordAgrees(report, 0, RelationKind) && Stigi.Store.Tests.kindWordAgrees(report, 0, RuleKind) && Stigi.Store.Tests.kindWordAgrees(report, 0, TestKind) && Stigi.Store.Tests.kindWordAgrees(report, 0, NegTestKind)
  Stigi.Test.expect(agrees, true, "every kind word size equals the bytes written")
