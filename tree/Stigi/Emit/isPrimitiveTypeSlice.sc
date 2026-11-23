// prompt: check whether an L0 type slice names a built-in scalar or pointer type.
def Stigi.Emit.isPrimitiveTypeSlice(srcCap: U64, srcBytes: Array[U8](srcCap), typeStart: U64, typeEnd: U64) <lessOrEqual(typeStart, typeEnd), lessOrEqual(typeEnd, srcCap), below(srcCap, 0x100000000)>: Bool =
  val isUnsigned: Bool = Stigi.Parse.sliceEquals(srcCap, srcBytes, typeStart, typeEnd, 2, "U8") || Stigi.Parse.sliceEquals(srcCap, srcBytes, typeStart, typeEnd, 3, "U32") || Stigi.Parse.sliceEquals(srcCap, srcBytes, typeStart, typeEnd, 3, "U64")
  val isSigned: Bool = Stigi.Parse.sliceEquals(srcCap, srcBytes, typeStart, typeEnd, 3, "I32") || Stigi.Parse.sliceEquals(srcCap, srcBytes, typeStart, typeEnd, 3, "I64")
  val isOther: Bool = Stigi.Parse.sliceEquals(srcCap, srcBytes, typeStart, typeEnd, 4, "Bool") || Stigi.Parse.sliceEquals(srcCap, srcBytes, typeStart, typeEnd, 4, "Unit") || Stigi.Parse.sliceEquals(srcCap, srcBytes, typeStart, typeEnd, 3, "Ptr") || Stigi.Parse.sliceEquals(srcCap, srcBytes, typeStart, typeEnd, 5, "Array")
  isUnsigned || isSigned || isOther
