// prompt: check that every decision arm body in a span is strictly below its parent postorder index.
def Stigi.Core.validateArmSpan(nodeAt: U64, armCap: U64, arms: Array[Stigi.Core.Arm](armCap), armCount: U64, span: Stigi.Core.Span) <lessOrEqual(armCount, armCap)>: Bool =
  span match
    case CoreSpan(first, endPos) =>
      if endPos > armCount then
        false
      else
        var scanIdx: U64 = first
        var validFlag: Bool = true
        while scanIdx < endPos && validFlag do
          @<lessOrEqual(first, scanIdx), lessOrEqual(scanIdx, endPos)>
          val nextScan: U64 = scanIdx + 1
          val bodyAt: U64 = arms(scanIdx) match
            case CaseArm(_, _, bodyPos) => bodyPos
            case ValueArm(_, _, bodyPos) => bodyPos
            case RestArm(bodyPos) => bodyPos
          if bodyAt >= nodeAt then
            validFlag = false
          else
            ()
          scanIdx = nextScan
        validFlag
