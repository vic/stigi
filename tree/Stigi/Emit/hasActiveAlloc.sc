// prompt: check whether any enclosing block stack frame holds an active heap array allocation.
def Stigi.Emit.hasActiveAlloc(blkKinds: Array[U8](96), blkPrev: Array[U64](96), blkTop: U64) <lessOrEqual(0, blkTop)>: Bool =
  var curIdx: U64 = blkTop
  var foundAlloc: Bool = false
  var walkStep: U64 = 0
  while walkStep < 96 && curIdx < 96 && !foundAlloc do
    @<lessOrEqual(walkStep, 96), decreases(96 - walkStep)>
    if blkKinds(curIdx) == 7 then
      foundAlloc = true
    else
      ()
    curIdx = blkPrev(curIdx)
    walkStep = walkStep + 1
  foundAlloc
