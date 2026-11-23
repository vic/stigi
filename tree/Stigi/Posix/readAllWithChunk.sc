// prompt: read an entire file descriptor into destBuf starting at destOffset using reusable scratch buffers.
pub def Stigi.Posix.readAllWithChunk {Stigi.Mut} (descriptor: I32, chunkBuf: Array[U8](65536), stateBox: Array[U64](2), destCap: U64, destBuf: Array[U8](destCap), destOffset: U64) <lessOrEqual(destOffset, destCap), below(destCap, 0x100000000)>: Stigi.Posix.ReadOutcome =
  stateBox(0) := 0
  stateBox(1) := 0
  var stepCount: U64 = 0
  while stepCount < 1024 && stateBox(1) == 0 do
    @<lessOrEqual(stepCount, 1024), decreases(1024 - stepCount)>
    val rawCount: I64 = Stigi.Posix.read(descriptor, chunkBuf, 65536)
    Stigi.Posix.classifyReadResult(rawCount, 65536) match
      case ReadFailed =>
        stateBox(1) := 2
      case ReadBytes(gotBytes) =>
        if gotBytes == 0 then
          stateBox(1) := 1
        else
          val curTotal: U64 = stateBox(0)
          if curTotal <= destCap then
            val writeBase: U64 = destOffset + curTotal
            if writeBase <= destCap then
              val remCap: U64 = destCap - writeBase
              if gotBytes <= remCap && gotBytes <= 65536 then
                var copyIdx: U64 = 0
                while copyIdx < gotBytes do
                  @<lessOrEqual(copyIdx, gotBytes), decreases(gotBytes - copyIdx)>
                  val dstPos: U64 = writeBase + copyIdx
                  if dstPos < destCap then
                    destBuf(dstPos) := chunkBuf(copyIdx)
                  else
                    ()
                  copyIdx = copyIdx + 1
                stateBox(0) := curTotal + gotBytes
              else
                stateBox(1) := 2
            else
              stateBox(1) := 2
          else
            stateBox(1) := 2
    stepCount = stepCount + 1
  if stateBox(1) == 1 then
    ReadBytes(stateBox(0))
  else
    ReadFailed
