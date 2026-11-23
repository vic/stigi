// prompt: check whether any enclosing block stack frame holds an active heap array allocation.
def Stigi.Emit.hasActiveAlloc(blkKinds: Array[U8](32), blkPrev: Array[U64](32), blkTop: U64) <lessOrEqual(0, blkTop)>: Bool =
  var curIdx: U64 = blkTop
  var foundAlloc: Bool = false
  var walkStep: U64 = 0
  while walkStep < 32 && curIdx < 32 && !foundAlloc do
    @<lessOrEqual(walkStep, 32), decreases(32 - walkStep)>
    if blkKinds(curIdx) == 7 then
      foundAlloc = true
    else
      ()
    curIdx = blkPrev(curIdx)
    walkStep = walkStep + 1
  foundAlloc
