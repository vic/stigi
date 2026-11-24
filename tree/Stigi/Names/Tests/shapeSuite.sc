// prompt: verify the name hash and the unit kinds the names phase reads from parsed units.
val Stigi.Names.Tests.shapeSuite {Stigi.Test}: Stigi.Test.Result =
  val passed: Bool =
    val Allocated(tables) = Stigi.Syntax.allocate{Stigi.Region}() else
      case AllocationFailed => false
    val upper: Array[U8](8) = "Stigi.Ab"
    val lower: Array[U8](8) = "stigi.ab"
    val seed: U64 = 14695981039346656037
    val foldedSame: Bool = Stigi.Names.textHash(8, upper, 0, 8, true, seed, 8) == Stigi.Names.textHash(8, lower, 0, 8, true, seed, 8)
    val exactDiffer: Bool = Stigi.Names.textHash(8, upper, 0, 8, false, seed, 8) != Stigi.Names.textHash(8, lower, 0, 8, false, seed, 8)
    val testText: Array[U8](86) = "// prompt: demo.\nval Demo.Tests.one {Stigi.Test}: Stigi.Test.Result =\n  Demo.check(1)\n"
    val testKind: Bool = Stigi.Syntax.Parse.parseUnit(tables, 86, testText, 86) match
      case Parsed(root, _, _) => Stigi.Names.shapeOfUnit(tables, root) is Shaped(TestKind, false)
      case ParseRejected(_, _) | LexRejected(_) => false
    val lemmaText: Array[U8](77) = "// prompt: demo.\npub lemma def Demo.law(x: U64) <below(x, 9)>: <below(x, 9)>\n"
    val lemmaKind: Bool = Stigi.Syntax.Parse.parseUnit(tables, 77, lemmaText, 77) match
      case Parsed(root, _, _) => Stigi.Names.shapeOfUnit(tables, root) is Shaped(DefKind, true)
      case ParseRejected(_, _) | LexRejected(_) => false
    val enumText: Array[U8](55) = "// prompt: demo.\nenum Demo.Pick:\n  case One\n  case Two\n"
    val enumKind: Bool = Stigi.Syntax.Parse.parseUnit(tables, 55, enumText, 55) match
      case Parsed(root, _, _) => Stigi.Names.shapeOfUnit(tables, root) is Shaped(EnumKind, false)
      case ParseRejected(_, _) | LexRejected(_) => false
    foldedSame && exactDiffer && testKind && lemmaKind && enumKind
  Stigi.Test.expect(passed, true, "the name hash folds case only when asked, and Test, lemma and enum units get their unit kinds")
