// prompt: classify native word bit-operation and cast builtin calls.
def Stigi.Emit.classifyWordBuiltin(srcCap: U64, srcBytes: Array[U8](srcCap), nameStart: U64, nameEnd: U64) <lessOrEqual(nameStart, nameEnd), lessOrEqual(nameEnd, srcCap), below(srcCap, 0x100000000)>: Stigi.Emit.BuiltinTag =
  if Stigi.Parse.sliceEquals(srcCap, srcBytes, nameStart, nameEnd, 8, "U64.ofU8") || Stigi.Parse.sliceEquals(srcCap, srcBytes, nameStart, nameEnd, 9, "U64.ofU32") || Stigi.Parse.sliceEquals(srcCap, srcBytes, nameStart, nameEnd, 9, "U64.ofI32") || Stigi.Parse.sliceEquals(srcCap, srcBytes, nameStart, nameEnd, 9, "U64.ofI64") then
    BuiltinInfo(3, 0)
  else if Stigi.Parse.sliceEquals(srcCap, srcBytes, nameStart, nameEnd, 8, "U8.ofU64") then
    BuiltinInfo(4, 0)
  else if Stigi.Parse.sliceEquals(srcCap, srcBytes, nameStart, nameEnd, 9, "U32.ofU64") then
    BuiltinInfo(5, 0)
  else if Stigi.Parse.sliceEquals(srcCap, srcBytes, nameStart, nameEnd, 9, "I32.ofU64") then
    BuiltinInfo(6, 0)
  else if Stigi.Parse.sliceEquals(srcCap, srcBytes, nameStart, nameEnd, 9, "I64.ofU64") then
    BuiltinInfo(7, 0)
  else if Stigi.Parse.sliceEquals(srcCap, srcBytes, nameStart, nameEnd, 7, "U64.mix") then
    BuiltinInfo(8, 0)
  else if Stigi.Parse.sliceEquals(srcCap, srcBytes, nameStart, nameEnd, 15, "U64.wrappingAdd") then
    BuiltinInfo(9, 1)
  else if Stigi.Parse.sliceEquals(srcCap, srcBytes, nameStart, nameEnd, 20, "U64.wrappingSubtract") then
    BuiltinInfo(9, 2)
  else if Stigi.Parse.sliceEquals(srcCap, srcBytes, nameStart, nameEnd, 20, "U64.wrappingMultiply") then
    BuiltinInfo(9, 3)
  else if Stigi.Parse.sliceEquals(srcCap, srcBytes, nameStart, nameEnd, 14, "U64.bitwiseAnd") then
    BuiltinInfo(9, 4)
  else if Stigi.Parse.sliceEquals(srcCap, srcBytes, nameStart, nameEnd, 13, "U64.bitwiseOr") then
    BuiltinInfo(9, 5)
  else if Stigi.Parse.sliceEquals(srcCap, srcBytes, nameStart, nameEnd, 14, "U64.bitwiseXor") then
    BuiltinInfo(9, 6)
  else if Stigi.Parse.sliceEquals(srcCap, srcBytes, nameStart, nameEnd, 13, "U64.shiftLeft") then
    BuiltinInfo(10, 1)
  else if Stigi.Parse.sliceEquals(srcCap, srcBytes, nameStart, nameEnd, 14, "U64.shiftRight") then
    BuiltinInfo(10, 2)
  else if Stigi.Parse.sliceEquals(srcCap, srcBytes, nameStart, nameEnd, 15, "U32.wrappingAdd") then
    BuiltinInfo(11, 1)
  else if Stigi.Parse.sliceEquals(srcCap, srcBytes, nameStart, nameEnd, 20, "U32.wrappingSubtract") then
    BuiltinInfo(11, 2)
  else if Stigi.Parse.sliceEquals(srcCap, srcBytes, nameStart, nameEnd, 20, "U32.wrappingMultiply") then
    BuiltinInfo(11, 3)
  else if Stigi.Parse.sliceEquals(srcCap, srcBytes, nameStart, nameEnd, 14, "U32.bitwiseAnd") then
    BuiltinInfo(11, 4)
  else if Stigi.Parse.sliceEquals(srcCap, srcBytes, nameStart, nameEnd, 13, "U32.bitwiseOr") then
    BuiltinInfo(11, 5)
  else if Stigi.Parse.sliceEquals(srcCap, srcBytes, nameStart, nameEnd, 14, "U32.bitwiseXor") then
    BuiltinInfo(11, 6)
  else if Stigi.Parse.sliceEquals(srcCap, srcBytes, nameStart, nameEnd, 13, "U32.shiftLeft") then
    BuiltinInfo(12, 1)
  else if Stigi.Parse.sliceEquals(srcCap, srcBytes, nameStart, nameEnd, 14, "U32.shiftRight") then
    BuiltinInfo(12, 2)
  else if Stigi.Parse.sliceEquals(srcCap, srcBytes, nameStart, nameEnd, 15, "U32.rotateRight") then
    BuiltinInfo(13, 0)
  else
    BuiltinInfo(0, 0)
