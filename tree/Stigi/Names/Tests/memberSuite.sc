// prompt: verify that the member table finds every enum case and operation by its text, one member at a time, and ends on unknown texts.
val Stigi.Names.Tests.memberSuite {Stigi.Test}: Stigi.Test.Result =
  val passed: Bool =
    val Allocated(tables) = Stigi.Syntax.allocate{Stigi.Region}() else
      case AllocationFailed => false
    val Filled(pool) = Array.tryFill[U8]{Stigi.Region}(Stigi.Limit.maxIndexBytes, 0) else
      case FillFailed => false
    val Filled(table) = Array.tryFill[Stigi.Names.Member]{Stigi.Region}(Stigi.Names.maxMembers, Member(0, CaseMember, 0, 0)) else
      case FillFailed => false
    val Filled(slots) = Array.tryFill[Stigi.Names.KeySlot]{Stigi.Region}(Stigi.Names.tableSlots, Vacant) else
      case FillFailed => false
    val enumText: Array[U8](55) = "// prompt: demo.\nenum Demo.Pick:\n  case One\n  case Two\n"
    val capabilityText: Array[U8](63) = "// prompt: demo.\ncapability Demo.Port:\n  def send(x: U64): U64\n"
    val otherText: Array[U8](45) = "// prompt: demo.\nenum Demo.Other:\n  case Two\n"
    val probeText: Array[U8](70) = "// prompt: demo.\ndef Demo.p(x: U64): U64 =\n  Two + One + send + Three\n"
    val empty: Stigi.Names.Members = Members(pool, 0, table, 0, slots)
    Stigi.Names.Tests.recordUnit(empty, tables, 55, enumText, 55, 0) match
      case Recorded(one) => Stigi.Names.Tests.recordUnit(one, tables, 63, capabilityText, 63, 1) match
        case Recorded(two) => Stigi.Names.Tests.recordUnit(two, tables, 45, otherText, 45, 2) match
          case Recorded(three) => Stigi.Syntax.Parse.parseUnit(tables, 70, probeText, 70) match
            case Parsed(_, _, _) =>
              three match
                case Members(_, _, hitTable, hitCount, _) =>
                  val sharedEnds: Bool = Stigi.Names.firstMember(three, tables, 45, 48) match
                    case MemberHit(firstHit, nextSlot) => Stigi.Names.nextMember(three, tables, 45, 48, nextSlot, Stigi.Names.tableSlots) match
                      case MemberHit(secondHit, lastSlot) =>
                        val firstOwner: U64 = if firstHit < hitCount then hitTable(firstHit) match
                          case Member(memberOwner, kind, _, _) => if kind is CaseMember then memberOwner else 99
                        else 99
                        val secondOwner: U64 = if secondHit < hitCount then hitTable(secondHit) match
                          case Member(memberOwner, kind, _, _) => if kind is CaseMember then memberOwner else 99
                        else 99
                        (firstOwner == 0 && secondOwner == 2 || firstOwner == 2 && secondOwner == 0) && Stigi.Names.nextMember(three, tables, 45, 48, lastSlot, Stigi.Names.tableSlots) is MembersEnded
                      case MembersEnded => false
                    case MembersEnded => false
                  val oneOnly: Bool = Stigi.Names.firstMember(three, tables, 51, 54) match
                    case MemberHit(hit, nextSlot) =>
                      val oneOwner: U64 = if hit < hitCount then hitTable(hit) match
                        case Member(memberOwner, kind, _, _) => if kind is CaseMember then memberOwner else 99
                      else 99
                      oneOwner == 0 && Stigi.Names.nextMember(three, tables, 51, 54, nextSlot, Stigi.Names.tableSlots) is MembersEnded
                    case MembersEnded => false
                  val sendOnly: Bool = Stigi.Names.firstMember(three, tables, 57, 61) match
                    case MemberHit(hit, nextSlot) =>
                      val sendOwner: U64 = if hit < hitCount then hitTable(hit) match
                        case Member(memberOwner, kind, _, _) => if kind is CaseMember then 99 else memberOwner
                      else 99
                      sendOwner == 1 && Stigi.Names.nextMember(three, tables, 57, 61, nextSlot, Stigi.Names.tableSlots) is MembersEnded
                    case MembersEnded => false
                  sharedEnds && oneOnly && sendOnly && Stigi.Names.firstMember(three, tables, 64, 69) is MembersEnded
            case ParseRejected(_, _) | LexRejected(_) => false
          case MembersFull | MembersMalformed => false
        case MembersFull | MembersMalformed => false
      case MembersFull | MembersMalformed => false
  Stigi.Test.expect(passed, true, "the member table finds enum cases and operations by their text one at a time and ends on an unknown text")
