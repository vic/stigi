// prompt: read chunks from an open file into a destination slice from a write position until the end of the file.
def Stigi.Posix.readChunks {file: Stigi.File, Stigi.Mut} (chunkBuf: Array[U8](4096), outCap: Stigi.ByteCap, outBuf: Array[U8](outCap), startOff: U64, writePos: U64, remaining: U64) <at(file, Open), lessOrEqual(startOff, writePos), lessOrEqual(writePos, outCap), decreases(remaining)>: Stigi.Posix.LoadResult <at(file, Open)> =
  if remaining < 1 then LoadFailed else
    file.readChunk(4096, chunkBuf) match
      case ReadEnd => LoadedBytes(writePos - startOff)
      case ReadFailed => LoadFailed
      case ReadBytes(gotLen) =>
        if gotLen <= 4096 then
          val endPos: U64 = Stigi.Array.copy(4096, chunkBuf, 0, gotLen, outCap, outBuf, writePos)
          if endPos - writePos < gotLen then LoadOverflow else Stigi.Posix.readChunks{file}(chunkBuf, outCap, outBuf, startOff, endPos, remaining - 1)
        else LoadFailed
