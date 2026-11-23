// prompt: emit the C type corresponding to an L0 type name slice in source bytes.
def Stigi.Emit.emitTypeSlice {Stigi.Mut} (srcCap: U64, srcBytes: Array[U8](srcCap), typeStart: U64, typeEnd: U64, outCap: U64, output: Array[U8](outCap), writeOffset: U64) <lessOrEqual(typeStart, typeEnd), lessOrEqual(typeEnd, srcCap), below(srcCap, 0x100000000), lessOrEqual(writeOffset, outCap), below(outCap, 0x100000000)>: U64 <lessOrEqual(writeOffset, result), lessOrEqual(result, outCap)> =
  if Stigi.Parse.sliceEquals(srcCap, srcBytes, typeStart, typeEnd, 2, "U8") then
    Stigi.Emit.writeLiteral(7, "uint8_t", outCap, output, writeOffset)
  else if Stigi.Parse.sliceEquals(srcCap, srcBytes, typeStart, typeEnd, 3, "U32") then
    Stigi.Emit.writeLiteral(8, "uint32_t", outCap, output, writeOffset)
  else if Stigi.Parse.sliceEquals(srcCap, srcBytes, typeStart, typeEnd, 3, "U64") then
    Stigi.Emit.writeLiteral(8, "uint64_t", outCap, output, writeOffset)
  else if Stigi.Parse.sliceEquals(srcCap, srcBytes, typeStart, typeEnd, 3, "I32") then
    Stigi.Emit.writeLiteral(7, "int32_t", outCap, output, writeOffset)
  else if Stigi.Parse.sliceEquals(srcCap, srcBytes, typeStart, typeEnd, 3, "I64") then
    Stigi.Emit.writeLiteral(7, "int64_t", outCap, output, writeOffset)
  else if Stigi.Parse.sliceEquals(srcCap, srcBytes, typeStart, typeEnd, 4, "Bool") then
    Stigi.Emit.writeLiteral(4, "bool", outCap, output, writeOffset)
  else if Stigi.Parse.sliceEquals(srcCap, srcBytes, typeStart, typeEnd, 4, "Unit") then
    Stigi.Emit.writeLiteral(4, "void", outCap, output, writeOffset)
  else if Stigi.Parse.sliceEquals(srcCap, srcBytes, typeStart, typeEnd, 3, "Ptr") then
    Stigi.Emit.writeLiteral(6, "void *", outCap, output, writeOffset)
  else if Stigi.Parse.sliceEquals(srcCap, srcBytes, typeStart, typeEnd, 17, "Stigi.Test.Result") then
    Stigi.Emit.writeLiteral(3, "int", outCap, output, writeOffset)
  else
    Stigi.Emit.writeSliceMangled(srcCap, srcBytes, typeStart, typeEnd, true, outCap, output, writeOffset)
