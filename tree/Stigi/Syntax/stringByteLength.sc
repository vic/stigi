// prompt: count the decoded bytes of a quoted string literal, rejecting malformed escapes or unclosed quotes.
pub def Stigi.Syntax.stringByteLength(srcCap: U64, source: Array[U8](srcCap), first: U64, endPos: U64) <lessOrEqual(first, endPos), lessOrEqual(endPos, srcCap)>: Stigi.Syntax.StringLength =
  if 0 < endPos then
    val closeAt: U64 = endPos - 1
    if first < closeAt then
      if U64.ofU8(source(first)) == 34 && U64.ofU8(source(closeAt)) == 34 then
        var pos: U64 = first + 1
        var byteCount: U64 = 0
        var malformed: Bool = false
        while pos < closeAt && !malformed do
          @<lessOrEqual(pos, closeAt)>
          if U64.ofU8(source(pos)) == 92 then
            val escAt: U64 = pos + 1
            if escAt < closeAt then
              val esc: U64 = U64.ofU8(source(escAt))
              if esc == 120 then
                val highAt: U64 = escAt + 1
                if highAt < closeAt then
                  val lowAt: U64 = highAt + 1
                  if lowAt < closeAt then
                    Stigi.Lex.decodeHex(source(highAt)) match
                      case NotHex => malformed = true
                      case HexValue(_) =>
                        Stigi.Lex.decodeHex(source(lowAt)) match
                          case NotHex => malformed = true
                          case HexValue(_) =>
                            if byteCount < Stigi.Limit.maxUnitBytes then byteCount = byteCount + 1 else malformed = true
                            pos = lowAt + 1
                  else malformed = true
                else malformed = true
              else
                Stigi.Syntax.simpleEscape(esc) match
                  case CharMalformed => malformed = true
                  case CharDecoded(_) =>
                    if byteCount < Stigi.Limit.maxUnitBytes then byteCount = byteCount + 1 else malformed = true
                    pos = escAt + 1
            else malformed = true
          else
            if byteCount < Stigi.Limit.maxUnitBytes then byteCount = byteCount + 1 else malformed = true
            pos = pos + 1
        if malformed then MalformedString else DecodedLength(byteCount)
      else MalformedString
    else MalformedString
  else MalformedString
