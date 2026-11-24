// prompt: intern Array of U8 with length n plus one built in both orders, with length n, and with another binder m, and report whether the two sums share one index while n and m hash and intern apart.
def Stigi.Types.Tests.linearProbe {region: Stigi.Region, Stigi.Mut} (slotCapacity: U64) <below(0, slotCapacity)>: Bool =
  val TableReady(table) = Stigi.Types.allocateTable{region}(8, slotCapacity, 8, 8, 8) else
    case TableUnavailable => false
  val Filled(pool) = Array.tryFill[Stigi.Types.Linear.Term]{region}(8, LinearTerm(0, WideInt(0, 0))) else
    case FillFailed => false
  pool(0) := LinearTerm(5, WideInt(1, 0))
  pool(1) := LinearTerm(6, WideInt(1, 0))
  val formBinder: Stigi.Types.Linear.Form = LinearForm(WideInt(0, 0), 0, 1)
  val formOther: Stigi.Types.Linear.Form = LinearForm(WideInt(0, 0), 1, 1)
  val formOne: Stigi.Types.Linear.Form = LinearForm(WideInt(1, 0), 0, 0)
  val arrayUnit: Stigi.Core.Reference = ExternalRef(4, 0)
  val LinearVal(sumLeft, poolAfter) = Stigi.Types.Linear.combine(8, pool, formBinder, formOne, false, 2) else
    case LinearNotLinear | LinearOverflow | LinearDivZero | LinearPoolFull => false
  if poolAfter <= 8 then
    val LinearVal(sumRight, _) = Stigi.Types.Linear.combine(8, pool, formOne, formBinder, false, poolAfter) else
      case LinearNotLinear | LinearOverflow | LinearDivZero | LinearPoolFull => false
    val InternedAt(withByte, byteAt) = Stigi.Types.intern(table, Primitive(UnsignedByte)) else
      case TypeTableFull => false
    val StagedRows(withArgument, argumentRows) = Stigi.Types.stageArgument(withByte, byteAt) else
      case StageFull | StageNotUnsigned | StageBadBinder => false
    val StagedRows(withLeftLength, leftRows) = Stigi.Types.stageLength(withArgument, 8, pool, sumLeft) else
      case StageFull | StageNotUnsigned | StageBadBinder => false
    val InternedAt(afterLeft, leftAt) = Stigi.Types.intern(withLeftLength, Named(arrayUnit, argumentRows, leftRows)) else
      case TypeTableFull => false
    val StagedRows(withRightLength, rightRows) = Stigi.Types.stageLength(afterLeft, 8, pool, sumRight) else
      case StageFull | StageNotUnsigned | StageBadBinder => false
    val InternedAt(afterRight, rightAt) = Stigi.Types.intern(withRightLength, Named(arrayUnit, argumentRows, rightRows)) else
      case TypeTableFull => false
    val StagedRows(withPlainLength, plainRows) = Stigi.Types.stageLength(afterRight, 8, pool, formBinder) else
      case StageFull | StageNotUnsigned | StageBadBinder => false
    val InternedAt(afterPlain, plainAt) = Stigi.Types.intern(withPlainLength, Named(arrayUnit, argumentRows, plainRows)) else
      case TypeTableFull => false
    val StagedRows(withOtherLength, otherRows) = Stigi.Types.stageLength(afterPlain, 8, pool, formOther) else
      case StageFull | StageNotUnsigned | StageBadBinder => false
    val hashesApart: Bool = Stigi.Types.namedHash(withOtherLength, arrayUnit, argumentRows, plainRows) != Stigi.Types.namedHash(withOtherLength, arrayUnit, argumentRows, otherRows)
    val InternedAt(finalTable, otherAt) = Stigi.Types.intern(withOtherLength, Named(arrayUnit, argumentRows, otherRows)) else
      case TypeTableFull => false
    hashesApart && leftAt == rightAt && leftAt != plainAt && plainAt != otherAt && !Stigi.Types.sameType(finalTable, leftAt, Named(arrayUnit, argumentRows, plainRows)) && !Stigi.Types.sameType(finalTable, plainAt, Named(arrayUnit, argumentRows, otherRows))
  else false
