// prompt: compute exact unsigned byte-order ranks for all interned symbols via iterative bottom-up merge sort.
def Stigi.Datalog.Store.computeRanks {Stigi.Mut} (symCap: U64, poolCap: U64, pool: Array[U8](poolCap), starts: Array[U64](symCap), ends: Array[U64](symCap), order: Array[U64](symCap), scratch: Array[U64](symCap), ranks: Array[U64](symCap), symCount: U64) <below(0, symCap), lessOrEqual(symCount, symCap)>: U64 =
  var initIdx: U64 = 0
  while initIdx < symCount do
    @<lessOrEqual(initIdx, symCount)>
    val nextInit: U64 = initIdx + 1
    order(initIdx) := initIdx
    initIdx = nextInit
  var width: U64 = 1
  while width < symCount do
    var leftStart: U64 = 0
    while leftStart < symCount do
      val candMid: U64 = U64.wrappingAdd(leftStart, width)
      val midPos: U64 = if candMid >= leftStart && candMid < symCount then candMid else symCount
      val candRight: U64 = U64.wrappingAdd(midPos, width)
      val rightEnd: U64 = if candRight >= midPos && candRight < symCount then candRight else symCount
      var leftIdx: U64 = leftStart
      var rightIdx: U64 = midPos
      var outIdx: U64 = leftStart
      while outIdx < rightEnd do
        val nextOut: U64 = outIdx + 1
        if leftIdx < midPos && rightIdx < rightEnd then
          val leftSym: U64 = order(leftIdx)
          val rightSym: U64 = order(rightIdx)
          val takeLeft: Bool =
            if leftSym < symCap && rightSym < symCap then
              val leftFirst: U64 = starts(leftSym)
              val leftLast: U64 = ends(leftSym)
              val rightFirst: U64 = starts(rightSym)
              val rightLast: U64 = ends(rightSym)
              if leftFirst <= leftLast && leftLast <= poolCap && rightFirst <= rightLast && rightLast <= poolCap then
                Stigi.Datalog.Store.compareSymbolBytes(poolCap, pool, leftFirst, leftLast, rightFirst, rightLast) <= 0
              else
                true
            else
              true
          if takeLeft then
            val nextLeft: U64 = leftIdx + 1
            scratch(outIdx) := leftSym
            leftIdx = nextLeft
          else
            val nextRight: U64 = rightIdx + 1
            scratch(outIdx) := rightSym
            rightIdx = nextRight
        else if leftIdx < midPos then
          val nextLeft: U64 = leftIdx + 1
          scratch(outIdx) := order(leftIdx)
          leftIdx = nextLeft
        else if rightIdx < rightEnd then
          val nextRight: U64 = rightIdx + 1
          scratch(outIdx) := order(rightIdx)
          rightIdx = nextRight
        else
          ()
        outIdx = nextOut
      var copyIdx: U64 = leftStart
      while copyIdx < rightEnd do
        val nextCopy: U64 = copyIdx + 1
        order(copyIdx) := scratch(copyIdx)
        copyIdx = nextCopy
      leftStart = rightEnd
    val candWidth: U64 = U64.wrappingAdd(width, width)
    if candWidth > width && candWidth < symCount then
      width = candWidth
    else
      width = symCount
  var posIdx: U64 = 0
  while posIdx < symCount do
    @<lessOrEqual(posIdx, symCount)>
    val nextPos: U64 = posIdx + 1
    val symIdx: U64 = order(posIdx)
    if symIdx < symCap then
      ranks(symIdx) := posIdx
    else
      ()
    posIdx = nextPos
  symCount
