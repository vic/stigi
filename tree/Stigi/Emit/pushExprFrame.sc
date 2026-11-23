// prompt: push one frame entry onto the bounded 32-slot expression lowering stack without subtraction.
def Stigi.Emit.pushExprFrame {Stigi.Mut} (frameKinds: Array[U8](32), frameSubs: Array[U8](32), frameArgs: Array[U8](32), frameTargets: Array[U64](32), framePrev: Array[U64](32), topBox: Array[U64](1), frameKind: U8, subKind: U8, targetTok: U64) <lessOrEqual(0, 1)>: Unit =
  val curTop: U64 = topBox(0)
  if curTop == 32 then
    frameKinds(0) := frameKind
    frameSubs(0) := subKind
    frameArgs(0) := 0
    frameTargets(0) := targetTok
    framePrev(0) := 32
    topBox(0) := 0
  else if curTop < 31 then
    val nextTop: U64 = curTop + 1
    frameKinds(nextTop) := frameKind
    frameSubs(nextTop) := subKind
    frameArgs(nextTop) := 0
    frameTargets(nextTop) := targetTok
    framePrev(nextTop) := curTop
    topBox(0) := nextTop
  else
    ()
