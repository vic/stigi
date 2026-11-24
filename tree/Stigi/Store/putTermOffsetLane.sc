// prompt: append one termOffset row to a lane when capacity remains.
def Stigi.Store.putTermOffsetLane {region: Stigi.Region, Stigi.Mut} (lane: Stigi.Store.Lane, unitSlice: Stigi.Store.NameSlice, pair: Stigi.Store.LengthPair, offset: U64): Stigi.Store.LaneWrite =
  lane match
    case Lane(size, bytes, at) =>
      @<lessOrEqual(at, size)>
      unitSlice match
        case NameSlice(_, first, endPos) =>
          if Stigi.Store.termOffsetRowSize(first, endPos) <= size - at then
            LaneWritten(Lane(size, bytes, Stigi.Store.putTermOffsetRow{region}(size, bytes, at, unitSlice, pair, offset)))
          else LaneOverflow
