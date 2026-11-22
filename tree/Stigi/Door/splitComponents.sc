// prompt: partition a batch into S114 dependency-closed components using deterministic union-find.
pub def Stigi.Door.splitComponents {Stigi.Mut} (unitCount: U64, parentOf: Array[U64](unitCount), edgeCount: U64, edgeFrom: Array[U64](edgeCount), edgeTo: Array[U64](edgeCount)) <below(0, unitCount), below(unitCount, 4096), below(edgeCount, 65536)>: U64 <lessOrEqual(result, unitCount)> =
  var initIndex: U64 = 0
  while initIndex < unitCount do
    @<lessOrEqual(initIndex, unitCount), decreases(unitCount - initIndex)>
    parentOf(initIndex) := initIndex
    initIndex = initIndex + 1
  var edgeIndex: U64 = 0
  while edgeIndex < edgeCount do
    @<lessOrEqual(edgeIndex, edgeCount), decreases(edgeCount - edgeIndex)>
    val fromNode: U64 = edgeFrom(edgeIndex)
    val toNode: U64 = edgeTo(edgeIndex)
    if fromNode < unitCount && toNode < unitCount then
      val rootFrom: U64 = Stigi.Door.findRoot(unitCount, parentOf, fromNode)
      val rootTo: U64 = Stigi.Door.findRoot(unitCount, parentOf, toNode)
      if rootFrom < rootTo then
        parentOf(rootTo) := rootFrom
      else if rootTo < rootFrom then
        parentOf(rootFrom) := rootTo
      else
        ()
    else
      ()
    edgeIndex = edgeIndex + 1
  var normIndex: U64 = 0
  var componentCount: U64 = 0
  while normIndex < unitCount do
    @<lessOrEqual(normIndex, unitCount), lessOrEqual(componentCount, normIndex), decreases(unitCount - normIndex)>
    val nextNorm: U64 = normIndex + 1
    val canonRoot: U64 = Stigi.Door.findRoot(unitCount, parentOf, normIndex)
    parentOf(normIndex) := canonRoot
    componentCount = if canonRoot == normIndex then componentCount + 1 else componentCount
    normIndex = nextNorm
  componentCount
