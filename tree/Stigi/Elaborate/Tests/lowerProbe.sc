// prompt: parse and lower one probe source with every pattern resolution equal to the given one and report its counts, first expression and first two arms.
def Stigi.Elaborate.Tests.lowerProbe {region: Stigi.Region, Stigi.Mut} (srcCap: U64, source: Array[U8](srcCap), res: Stigi.Elaborate.PatResolution) <lessOrEqual(srcCap, Stigi.Limit.maxUnitBytes)>: Stigi.Elaborate.Tests.Probe =
  val zeroWord: U64 = 0
  val initWork: Stigi.Elaborate.Work = Enter(0)
  val Allocated(tables) = Stigi.Syntax.allocate{region}() else
    case AllocationFailed => ProbeFillFailed
  val Filled(exprs) = Array.tryFill[Stigi.Core.Expr]{region}(16, NothingExpr(0)) else
    case FillFailed => ProbeFillFailed
  val Filled(arms) = Array.tryFill[Stigi.Core.Arm]{region}(4, RestArm(0)) else
    case FillFailed => ProbeFillFailed
  val Filled(childLists) = Array.tryFill[U64]{region}(8, zeroWord) else
    case FillFailed => ProbeFillFailed
  val Filled(work) = Array.tryFill[Stigi.Elaborate.Work]{region}(128, initWork) else
    case FillFailed => ProbeFillFailed
  val Filled(mapped) = Array.tryFill[U64]{region}(128, zeroWord) else
    case FillFailed => ProbeFillFailed
  val Filled(patRes) = Array.tryFill[Stigi.Elaborate.PatResolution]{region}(128, res) else
    case FillFailed => ProbeFillFailed
  Stigi.Syntax.Parse.parseUnit(tables, srcCap, source, srcCap) match
    case ParseRejected(_, _) => ProbeUnparsed
    case LexRejected(_) => ProbeUnparsed
    case Parsed(root, _, _) =>
      Stigi.Elaborate.lowerUnit(tables, root, CoreTables(16, exprs, 0, 0, 8, childLists, 0, 0, 4, arms, 0, 0), WorkScratch(128, work, 0, mapped, patRes)) match
        case LowerOverflow => ProbeOverflow
        case LowerInvalid => ProbeInvalid
        case Lowered(_, CoreTables(_, _, _, exprCount, _, _, _, _, _, _, _, armCount), _) => ProbeLowered(exprCount, armCount, exprs(0), arms(0), arms(1))
