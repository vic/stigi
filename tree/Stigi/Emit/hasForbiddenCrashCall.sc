// prompt: verify that emitted C source obeys the crash line by containing zero forbidden process termination calls.
pub def Stigi.Emit.hasForbiddenCrashCall(capacity: U64, text: Array[U8](capacity), length: U64) <lessOrEqual(length, capacity), below(capacity, 0x100000000)>: Bool =
  var cursor: U64 = 0
  var prevIdent: Bool = false
  var found: Bool = false
  while cursor < length && !found do
    @<lessOrEqual(cursor, length), decreases(length - cursor)>
    val currentByte: U8 = text(cursor)
    if !prevIdent then
      val matchFive: Bool = if cursor + 5 < length then currentByte == 97 && text(cursor + 1) == 98 && text(cursor + 2) == 111 && text(cursor + 3) == 114 && text(cursor + 4) == 116 && text(cursor + 5) == 40 else false
      val matchFour: Bool = if cursor + 4 < length then currentByte == 101 && text(cursor + 1) == 120 && text(cursor + 2) == 105 && text(cursor + 3) == 116 && text(cursor + 4) == 40 else false
      if matchFive || matchFour then
        found = true
      else
        ()
    else
      ()
    prevIdent = Stigi.Emit.isCIdentByte(currentByte)
    cursor = cursor + 1
  found
