// prompt: check that every child index in a span is strictly below its parent postorder index.
def Stigi.Core.validateChildSpan(nodeAt: U64, listCap: U64, childLists: Array[U64](listCap), listCount: U64, span: Stigi.Core.Span) <lessOrEqual(listCount, listCap)>: Bool =
  span match
    case CoreSpan(first, endPos) =>
      if endPos > listCount then
        false
      else
        var scanIdx: U64 = first
        var validFlag: Bool = true
        while scanIdx < endPos && validFlag do
          @<lessOrEqual(first, scanIdx), lessOrEqual(scanIdx, endPos)>
          val nextScan: U64 = scanIdx + 1
          if childLists(scanIdx) >= nodeAt then
            validFlag = false
          else
            ()
          scanIdx = nextScan
        validFlag
