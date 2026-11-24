// prompt: allocate reusable elaboration working scratch arrays of the given capacity.
pub def Stigi.Elaborate.allocateScratch {region: Stigi.Region} (capacity: U64) <lessOrEqual(capacity, Stigi.Limit.maxUnitNodes)>: Stigi.Elaborate.ScratchMade =
  val Filled(work) = Array.tryFill[Stigi.Elaborate.Work]{region}(capacity, Enter(0)) else
    case FillFailed => ScratchUnavailable
  val Filled(mapped) = Array.tryFill[U64]{region}(capacity, 0) else
    case FillFailed => ScratchUnavailable
  val Filled(patRes) = Array.tryFill[Stigi.Elaborate.PatResolution]{region}(capacity, PatLocalBind) else
    case FillFailed => ScratchUnavailable
  ScratchReady(WorkScratch(capacity, work, 0, mapped, patRes))
