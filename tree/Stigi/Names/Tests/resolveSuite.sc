// prompt: verify that name resolution prefers nearer units and nearer enum cases, applies visibility apart from the search, and names builtins.
val Stigi.Names.Tests.resolveSuite {Stigi.Test}: Stigi.Test.Result =
  val passed: Bool =
    val Allocated(tables) = Stigi.Syntax.allocate{Stigi.Region}() else
      case AllocationFailed => false
    val Filled(pool) = Array.tryFill[U8]{Stigi.Region}(Stigi.Limit.maxIndexBytes, 0) else
      case FillFailed => false
    val Filled(entries) = Array.tryFill[Stigi.Names.Entry]{Stigi.Region}(Stigi.Limit.maxUnits, Entry(0, 0, DefKind, false)) else
      case FillFailed => false
    val Filled(slots) = Array.tryFill[Stigi.Names.KeySlot]{Stigi.Region}(Stigi.Names.tableSlots, Vacant) else
      case FillFailed => false
    val Filled(memberPool) = Array.tryFill[U8]{Stigi.Region}(Stigi.Limit.maxIndexBytes, 0) else
      case FillFailed => false
    val Filled(table) = Array.tryFill[Stigi.Names.Member]{Stigi.Region}(Stigi.Names.maxMembers, Member(0, CaseMember, 0, 0)) else
      case FillFailed => false
    val Filled(memberSlots) = Array.tryFill[Stigi.Names.KeySlot]{Stigi.Region}(Stigi.Names.tableSlots, Vacant) else
      case FillFailed => false
    val names: Array[U8](47) = "A.B.c\nA.d\nA.B.d\nC.x\nC.y\nA.Pick\nD.Pick\nE.Pick\nd\n"
    val copied: U64 = Stigi.Array.copy[U8](47, names, 0, 47, Stigi.Limit.maxIndexBytes, pool, 0)
    val Added(alpha) = Stigi.Names.addEntry(Image(pool, entries, 0, slots), 0, 5, Shaped(DefKind, false)) else
      case ImageFull | UnitUnshaped => false
    val Added(beta) = Stigi.Names.addEntry(alpha, 6, 9, Shaped(DefKind, false)) else
      case ImageFull | UnitUnshaped => false
    val Added(gamma) = Stigi.Names.addEntry(beta, 10, 15, Shaped(DefKind, false)) else
      case ImageFull | UnitUnshaped => false
    val Added(delta) = Stigi.Names.addEntry(gamma, 16, 19, Shaped(DefKind, true)) else
      case ImageFull | UnitUnshaped => false
    val Added(epsilon) = Stigi.Names.addEntry(delta, 20, 23, Shaped(DefKind, false)) else
      case ImageFull | UnitUnshaped => false
    val Added(zeta) = Stigi.Names.addEntry(epsilon, 24, 30, Shaped(EnumKind, false)) else
      case ImageFull | UnitUnshaped => false
    val Added(eta) = Stigi.Names.addEntry(zeta, 31, 37, Shaped(EnumKind, true)) else
      case ImageFull | UnitUnshaped => false
    val Added(theta) = Stigi.Names.addEntry(eta, 38, 44, Shaped(EnumKind, true)) else
      case ImageFull | UnitUnshaped => false
    val Added(iota) = Stigi.Names.addEntry(theta, 45, 46, Shaped(DefKind, true)) else
      case ImageFull | UnitUnshaped => false
    val pickNear: Array[U8](52) = "// prompt: demo.\nenum A.Pick:\n  case One\n  case Two\n"
    val pickFar: Array[U8](54) = "// prompt: demo.\nenum D.Pick:\n  case Two\n  case Three\n"
    val pickOther: Array[U8](41) = "// prompt: demo.\nenum E.Pick:\n  case Two\n"
    val probe: Array[U8](107) = "// prompt: demo.\ndef A.B.c(x: U64): U64 =\n  d + x + C.x + C.y + zzz + One + Two + Three + A.Pick.One + B.d\n"
    val Recorded(one) = Stigi.Names.Tests.recordUnit(Members(memberPool, 0, table, 0, memberSlots), tables, 52, pickNear, 52, 5) else
      case MembersFull | MembersMalformed => false
    val Recorded(two) = Stigi.Names.Tests.recordUnit(one, tables, 54, pickFar, 54, 6) else
      case MembersFull | MembersMalformed => false
    val Recorded(three) = Stigi.Names.Tests.recordUnit(two, tables, 41, pickOther, 41, 7) else
      case MembersFull | MembersMalformed => false
    val Parsed(_, _, _) = Stigi.Syntax.Parse.parseUnit(tables, 107, probe, 107) else
      case ParseRejected(_, _) | LexRejected(_) => false
    val found: Bool = Stigi.Names.resolveText(NameScope(iota, three), tables, 0, Text(44, 45), false) is ToUnit(2) && Stigi.Names.resolveText(NameScope(iota, three), tables, 0, Text(52, 55), false) is ToUnit(3) && Stigi.Names.resolveText(NameScope(iota, three), tables, 0, Text(58, 61), false) is ToUnit(4) && Stigi.Names.visibleFrom(iota, 4, 0) == false && Stigi.Names.visibleFrom(iota, 3, 0)
    val missed: Bool = Stigi.Names.resolveText(NameScope(iota, three), tables, 0, Text(48, 49), false) is Unresolved && Stigi.Names.resolveText(NameScope(iota, three), tables, 0, Text(64, 67), true) is Unresolved && Stigi.Names.resolveText(NameScope(iota, three), tables, 0, Text(30, 33), false) is ToBuiltin && Stigi.Names.resolveText(NameScope(iota, three), tables, 3, Text(103, 106), true) is Unresolved
    val cases: Bool = Stigi.Names.resolveText(NameScope(iota, three), tables, 0, Text(70, 73), true) is ToCase(5, _) && Stigi.Names.resolveText(NameScope(iota, three), tables, 0, Text(76, 79), true) is ToCase(5, _) && Stigi.Names.resolveText(NameScope(iota, three), tables, 0, Text(90, 100), true) is ToCase(5, _) && Stigi.Names.resolveText(NameScope(iota, three), tables, 3, Text(82, 87), true) is ToCase(6, _) && Stigi.Names.resolveText(NameScope(iota, three), tables, 3, Text(76, 79), true) is AmbiguousCase
    copied == 47 && found && missed && cases
  Stigi.Test.expect(passed, true, "name resolution prefers nearer units and cases, leaves visibility to the walker, and names builtins")
