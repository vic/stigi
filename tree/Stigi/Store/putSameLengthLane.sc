// prompt: append one sameLength row to a lane when capacity remains.
def Stigi.Store.putSameLengthLane {region: Stigi.Region, Stigi.Mut} (lane: Stigi.Store.Lane, unitSlice: Stigi.Store.NameSlice, pair: Stigi.Store.LengthPair): Stigi.Store.LaneWrite =
  lane match
    case Lane(size, bytes, at) =>
      @<lessOrEqual(at, size)>
      unitSlice match
        case NameSlice(_, first, endPos) =>
          if Stigi.Store.sameLengthRowSize(first, endPos) <= size - at then
            LaneWritten(Lane(size, bytes, Stigi.Store.putSameLengthRow{region}(size, bytes, at, unitSlice, pair)))
          else LaneOverflow
