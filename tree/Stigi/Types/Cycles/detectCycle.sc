// prompt: detect recursive-type cycles over inline and element edges via iterative three-color DFS.
pub def Stigi.Types.Cycles.detectCycle {region: Stigi.Region, Stigi.Mut} (typeCap: U64, typeCount: U64, edgeCap: U64, edges: Array[Stigi.Types.Cycles.TypeEdge](edgeCap), edgeCount: U64) <below(0, typeCap), lessOrEqual(1, typeCap), lessOrEqual(typeCount, typeCap), lessOrEqual(edgeCount, edgeCap)>: Stigi.Types.Cycles.CycleResult =
  val zeroWord: U64 = 0
  Array.tryFill{region}(typeCap, zeroWord) match
    case FillFailed => CycleStackFull
    case Filled(colors) =>
      Array.tryFill{region}(typeCap, zeroWord) match
        case FillFailed => CycleStackFull
        case Filled(stackNode) =>
          Array.tryFill{region}(typeCap, zeroWord) match
            case FillFailed => CycleStackFull
            case Filled(stackEdge) =>
              var foundCycle: Bool = false
              var stackFull: Bool = false
              var cycFrom: U64 = 0
              var cycTo: U64 = 0
              var rootIdx: U64 = 0
              while rootIdx < typeCount && !foundCycle && !stackFull do
                @<lessOrEqual(rootIdx, typeCount)>
                val nextRoot: U64 = rootIdx + 1
                if colors(rootIdx) == 0 then
                  colors(rootIdx) := 1
                  stackNode(0) := rootIdx
                  stackEdge(0) := 0
                  var depth: U64 = 1
                  while depth > 0 && !foundCycle && !stackFull do
                    @<lessOrEqual(depth, typeCap)>
                    val topIdx: U64 = depth - 1
                    val curNode: U64 = stackNode(topIdx)
                    val curEdge: U64 = stackEdge(topIdx)
                    val nextDepthVal: U64 = if curEdge < edgeCount then
                      val nextEdge: U64 = curEdge + 1
                      stackEdge(topIdx) := nextEdge
                      var shouldPush: Bool = false
                      var pushTarget: U64 = 0
                      edges(curEdge) match
                        case DepEdge(fromType, toType, _) =>
                          if fromType == curNode && toType < typeCount then
                            val toColor: U64 = colors(toType)
                            if toColor == 1 then
                              foundCycle = true
                              cycFrom = fromType
                              cycTo = toType
                            else if toColor == 0 then
                              shouldPush = true
                              pushTarget = toType
                            else
                              ()
                          else
                            ()
                      if shouldPush then
                        if depth < typeCap then
                          val advDepth: U64 = depth + 1
                          if pushTarget < typeCap then
                            colors(pushTarget) := 1
                          else
                            ()
                          stackNode(depth) := pushTarget
                          stackEdge(depth) := 0
                          advDepth
                        else
                          stackFull = true
                          depth
                      else
                        depth
                    else
                      if curNode < typeCap then
                        colors(curNode) := 2
                      else
                        ()
                      topIdx
                    depth = nextDepthVal
                else
                  ()
                rootIdx = nextRoot
              if foundCycle then
                RecursiveCycle(cycFrom, cycTo)
              else if stackFull then
                CycleStackFull
              else
                AcyclicTypes
