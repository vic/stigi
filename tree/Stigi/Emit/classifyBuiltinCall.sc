// prompt: classify builtin, test assertion, and POSIX extern calls by name slice.
pub def Stigi.Emit.classifyBuiltinCall(srcCap: U64, srcBytes: Array[U8](srcCap), nameStart: U64, nameEnd: U64) <lessOrEqual(nameStart, nameEnd), lessOrEqual(nameEnd, srcCap), below(srcCap, 0x100000000)>: Stigi.Emit.BuiltinTag =
  val BuiltinInfo(wordKind, wordSub) = Stigi.Emit.classifyWordBuiltin(srcCap, srcBytes, nameStart, nameEnd)
  if wordKind != 0 then
    BuiltinInfo(wordKind, wordSub)
  else if Stigi.Parse.sliceEquals(srcCap, srcBytes, nameStart, nameEnd, 17, "Stigi.Test.expect") then
    BuiltinInfo(15, 0)
  else if Stigi.Parse.sliceEquals(srcCap, srcBytes, nameStart, nameEnd, 18, "Stigi.Posix.openat") then
    BuiltinInfo(17, 1)
  else if Stigi.Parse.sliceEquals(srcCap, srcBytes, nameStart, nameEnd, 16, "Stigi.Posix.read") then
    BuiltinInfo(17, 2)
  else if Stigi.Parse.sliceEquals(srcCap, srcBytes, nameStart, nameEnd, 17, "Stigi.Posix.write") then
    BuiltinInfo(17, 3)
  else if Stigi.Parse.sliceEquals(srcCap, srcBytes, nameStart, nameEnd, 17, "Stigi.Posix.lseek") then
    BuiltinInfo(17, 4)
  else if Stigi.Parse.sliceEquals(srcCap, srcBytes, nameStart, nameEnd, 17, "Stigi.Posix.close") then
    BuiltinInfo(17, 5)
  else if Stigi.Parse.sliceEquals(srcCap, srcBytes, nameStart, nameEnd, 18, "Stigi.Posix.unlink") then
    BuiltinInfo(17, 6)
  else if Stigi.Parse.sliceEquals(srcCap, srcBytes, nameStart, nameEnd, 21, "Stigi.Posix.getrusage") then
    BuiltinInfo(17, 7)
  else if Stigi.Parse.sliceEquals(srcCap, srcBytes, nameStart, nameEnd, 25, "Stigi.Posix.clock_gettime") then
    BuiltinInfo(17, 8)
  else
    BuiltinInfo(0, 0)
