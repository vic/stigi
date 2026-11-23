// prompt: pop any trailing inline else frames from the expression stack emitting closing parentheses.
def Stigi.Emit.popInlineElseFrames {Stigi.Mut} (frameKinds: Array[U8](32), framePrev: Array[U64](32), topBox: Array[U64](1), outCap: U64, output: Array[U8](outCap), writeOffset: U64) <lessOrEqual(writeOffset, outCap), below(outCap, 0x100000000)>: U64 <lessOrEqual(writeOffset, result), lessOrEqual(result, outCap)> =
  var pos: U64 = writeOffset
  var drainStep: U64 = 0
  var active: Bool = true
  while drainStep < 32 && active do
    @<lessOrEqual(drainStep, 32), lessOrEqual(writeOffset, pos), lessOrEqual(pos, outCap), decreases(32 - drainStep)>
    val curTop: U64 = topBox(0)
    if curTop < 32 then
      if frameKinds(curTop) == 20 then
        pos = Stigi.Emit.writeLiteral(2, "))", outCap, output, pos)
        topBox(0) := framePrev(curTop)
      else
        active = false
    else
      active = false
    drainStep = drainStep + 1
  pos
