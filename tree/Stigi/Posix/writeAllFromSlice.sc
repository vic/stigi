// prompt: write the entire byte slice srcBuf[srcStart..srcEnd] to descriptor handling partial writes.
pub def Stigi.Posix.writeAllFromSlice {region: Stigi.Region, Stigi.Mut} (descriptor: I32, srcCap: U64, srcBuf: Array[U8](srcCap), srcStart: U64, srcEnd: U64) <lessOrEqual(srcStart, srcEnd), lessOrEqual(srcEnd, srcCap), below(srcCap, 0x100000000)>: Stigi.Posix.WriteOutcome =
  val totalLen: U64 = srcEnd - srcStart
  val zeroByte: U8 = 0
  val zeroWord: U64 = 0
  if totalLen == 0 then
    WroteBytes(0)
  else
    Array.tryFill{region}(2, zeroWord) match
      case FillFailed => WriteFailed
      case Filled(stateBox) =>
        var stepCount: U64 = 0
        while stepCount < 2048 && stateBox(1) == 0 && stateBox(0) < totalLen do
          @<lessOrEqual(stepCount, 2048), decreases(2048 - stepCount)>
          val cursor: U64 = stateBox(0)
          if cursor < totalLen then
            val remBytes: U64 = totalLen - cursor
            val batchLen: U64 = if remBytes < 65536 then remBytes else 65536
            val copyBase: U64 = srcStart + cursor
            Array.tryFill{region}(batchLen, zeroByte) match
              case FillFailed =>
                stateBox(1) := 2
              case Filled(outChunk) =>
                var copyIdx: U64 = 0
                while copyIdx < batchLen do
                  @<lessOrEqual(copyIdx, batchLen), decreases(batchLen - copyIdx)>
                  val srcPos: U64 = copyBase + copyIdx
                  if srcPos < srcEnd then
                    outChunk(copyIdx) := srcBuf(srcPos)
                  else
                    ()
                  copyIdx = copyIdx + 1
                val rawWrote: I64 = Stigi.Posix.write(descriptor, outChunk, batchLen)
                Stigi.Posix.classifyWriteResult(rawWrote, batchLen) match
                  case WriteFailed =>
                    stateBox(1) := 2
                  case WroteBytes(wroteCnt) =>
                    if wroteCnt > 0 && wroteCnt <= remBytes then
                      stateBox(0) := cursor + wroteCnt
                    else
                      stateBox(1) := 2
          else
            ()
          stepCount = stepCount + 1
        if stateBox(1) == 0 && stateBox(0) == totalLen then
          WroteBytes(totalLen)
        else
          WriteFailed
