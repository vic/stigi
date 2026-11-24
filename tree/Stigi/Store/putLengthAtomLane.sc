// prompt: append one lengthTerm or linearForm row to a lane when capacity remains.
def Stigi.Store.putLengthAtomLane {region: Stigi.Region, Stigi.Mut} (lane: Stigi.Store.Lane, unitSlice: Stigi.Store.NameSlice, termIdx: U64): Stigi.Store.LaneWrite =
  lane match
    case Lane(size, bytes, at) =>
      @<lessOrEqual(at, size)>
      unitSlice match
        case NameSlice(_, first, endPos) =>
          if Stigi.Store.lengthAtomRowSize(first, endPos) <= size - at then
            LaneWritten(Lane(size, bytes, Stigi.Store.putLengthAtomRow{region}(size, bytes, at, unitSlice, termIdx)))
          else LaneOverflow
