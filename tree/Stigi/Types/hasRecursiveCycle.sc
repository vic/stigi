// prompt: detect recursive type cycles across inline enum fields, arrays, and cells.
pub def Stigi.Types.hasRecursiveCycle {region: Stigi.Region, Stigi.Mut} (typeCount: U64, edgeCount: U64, fieldFrom: Array[U64](edgeCount), fieldTo: Array[U64](edgeCount)) <below(0, typeCount), below(typeCount, 256), below(edgeCount, 65536)>: Bool =
  val zeroWord: U64 = 0
  val zeroByte: U8 = 0
  Array.tryFill{region}(typeCount, zeroWord) match
    case FillFailed => true
    case Filled(queue) =>
      Array.tryFill{region}(typeCount, zeroByte) match
        case FillFailed => true
        case Filled(seen) =>
          var foundCycle: Bool = false
          var startType: U64 = 0
          while startType < typeCount && !foundCycle do
            @<lessOrEqual(startType, typeCount), decreases(typeCount - startType)>
            var clearIdx: U64 = 0
            while clearIdx < typeCount do
              @<lessOrEqual(clearIdx, typeCount), decreases(typeCount - clearIdx)>
              seen(clearIdx) := 0
              clearIdx = clearIdx + 1
            var head: U64 = 0
            var tail: U64 = 0
            var edgeSeed: U64 = 0
            while edgeSeed < edgeCount do
              @<lessOrEqual(edgeSeed, edgeCount), lessOrEqual(tail, typeCount), decreases(edgeCount - edgeSeed)>
              if fieldFrom(edgeSeed) == startType then
                val nxt: U64 = fieldTo(edgeSeed)
                if nxt < typeCount && seen(nxt) == 0 && tail < typeCount then
                  seen(nxt) := 1
                  queue(tail) := nxt
                  tail = tail + 1
                else
                  ()
              else
                ()
              edgeSeed = edgeSeed + 1
            while head < tail do
              @<lessOrEqual(head, tail), lessOrEqual(tail, typeCount), decreases(typeCount - head)>
              val cur: U64 = queue(head)
              head = head + 1
              var edgeIdx: U64 = 0
              while edgeIdx < edgeCount do
                @<lessOrEqual(edgeIdx, edgeCount), lessOrEqual(head, tail), lessOrEqual(tail, typeCount), decreases(edgeCount - edgeIdx)>
                if fieldFrom(edgeIdx) == cur then
                  val nxt: U64 = fieldTo(edgeIdx)
                  if nxt < typeCount && seen(nxt) == 0 && tail < typeCount then
                    seen(nxt) := 1
                    queue(tail) := nxt
                    tail = tail + 1
                  else
                    ()
                else
                  ()
                edgeIdx = edgeIdx + 1
            if seen(startType) != 0 then
              foundCycle = true
            else
              ()
            startType = startType + 1
          foundCycle
