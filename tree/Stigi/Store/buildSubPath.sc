// prompt: build a child path by joining directory, slash, and suffix into outBuf.
pub def Stigi.Store.buildSubPath {Stigi.Mut} (dirCap: U64, dirBuf: Array[U8](dirCap), dirStart: U64, dirEnd: U64, suffixLen: U64, suffix: Array[U8](suffixLen), outCap: U64, outBuf: Array[U8](outCap)) <lessOrEqual(dirStart, dirEnd), lessOrEqual(dirEnd, dirCap), below(dirCap, 0x100000000), below(outCap, 0x100000000)>: U64 <lessOrEqual(result, outCap)> =
  var pos: U64 = Stigi.Print.writeSlice(dirCap, dirBuf, dirStart, dirEnd, outCap, outBuf, 0)
  pos = Stigi.Emit.writeByte(outCap, outBuf, pos, 47)
  Stigi.Emit.writeLiteral(suffixLen, suffix, outCap, outBuf, pos)
