// prompt: decode a character literal text that holds its two quotes: one raw byte or one escape between them.
pub def Stigi.Syntax.charValue(srcCap: U64, source: Array[U8](srcCap), first: U64, endPos: U64) <lessOrEqual(endPos, srcCap)>: Stigi.Syntax.CharDecoded =
  if first < endPos then
    val bodyAt: U64 = first + 1
    if U64.ofU8(source(first)) == 39 && bodyAt < endPos then
      val body: U64 = U64.ofU8(source(bodyAt))
      if body == 92 then
        val escAt: U64 = bodyAt + 1
        if escAt < endPos then
          val esc: U64 = U64.ofU8(source(escAt))
          if esc == 120 then
            val highAt: U64 = escAt + 1
            if highAt < endPos then
              val lowAt: U64 = highAt + 1
              if lowAt < endPos then
                Stigi.Lex.decodeHex(source(highAt)) match
                  case NotHex => CharMalformed
                  case HexValue(high) =>
                    Stigi.Lex.decodeHex(source(lowAt)) match
                      case NotHex => CharMalformed
                      case HexValue(low) => if Stigi.Syntax.closesAt(srcCap, source, lowAt + 1, endPos) then CharDecoded(U64.wrappingAdd(U64.wrappingMultiply(high, 16), low)) else CharMalformed
              else CharMalformed
            else CharMalformed
          else if Stigi.Syntax.closesAt(srcCap, source, escAt + 1, endPos) then Stigi.Syntax.simpleEscape(esc) else CharMalformed
        else CharMalformed
      else if Stigi.Syntax.closesAt(srcCap, source, bodyAt + 1, endPos) then CharDecoded(body) else CharMalformed
    else CharMalformed
  else CharMalformed
