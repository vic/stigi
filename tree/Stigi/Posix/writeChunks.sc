// prompt: write the bytes of a source slice from a position to an open file in chunks, the last one exactly sized.
def Stigi.Posix.writeChunks {file: Stigi.File, region: Stigi.Region, Stigi.Mut} (chunkBuf: Array[U8](4096), srcCap: Stigi.EmitCap, srcBuf: Array[U8](srcCap), srcLen: U64, sent: U64, remaining: U64) <at(file, Open), lessOrEqual(srcLen, srcCap), lessOrEqual(sent, srcLen), decreases(remaining)>: Stigi.RunOutcome <at(file, Open)> =
  if remaining < 1 then RunFileError else
    val chunkEnd: U64 = sent + 4096
    if chunkEnd <= srcLen then
      Stigi.Array.copy(srcCap, srcBuf, sent, chunkEnd, 4096, chunkBuf, 0)
      file.writeChunk(4096, chunkBuf) match
        case WroteBytes(wrote) =>
          if 0 < wrote && wrote <= 4096 then
            val next: U64 = sent + wrote
            if next <= srcLen then Stigi.Posix.writeChunks{file, region}(chunkBuf, srcCap, srcBuf, srcLen, next, remaining - 1) else RunFileError
          else RunFileError
        case WriteFailed => RunFileError
    else
      val rest: U64 = srcLen - sent
      if rest < 1 then RunSuccess else
        val Filled(tailBuf) = Array.tryFill[U8]{region}(rest, 0) else
          case FillFailed => RunOutOfMemory
        Stigi.Array.copy(srcCap, srcBuf, sent, srcLen, rest, tailBuf, 0)
        file.writeChunk(rest, tailBuf) match
          case WroteBytes(wrote) => if wrote == rest then RunSuccess else RunFileError
          case WriteFailed => RunFileError
