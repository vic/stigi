// prompt: implicit equality instance for closed lookup outcomes via multi-head matching.
pub given Stigi.Equal.Lookup: Stigi.Equal[Stigi.Lookup]:
  def equal(left: Stigi.Lookup, right: Stigi.Lookup): Bool =
    case (Found(leftSlot), Found(rightSlot)) => leftSlot == rightSlot
    case (Missing, Missing) => true
    case (Found(_), Missing) => false
    case (Missing, Found(_)) => false
