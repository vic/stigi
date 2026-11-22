// prompt: find the value span of a JSON string field inside a transport envelope without allocation.
pub def Stigi.Server.extractJsonStringField(length: U64, payload: Array[U8](length), keyLen: U64, keyName: Array[U8](keyLen)) <below(length, 0x100000000), below(keyLen, 256)>: Stigi.Server.EnvelopeOutcome =
  var cursor: U64 = 0
  var found: Bool = false
  var valStart: U64 = 0
  var valEnd: U64 = 0
  while cursor < length && !found do
    @<lessOrEqual(cursor, length), lessOrEqual(valStart, valEnd), lessOrEqual(valEnd, length), decreases(length - cursor)>
    if payload(cursor) == 34 then
      val keyStart: U64 = cursor + 1
      val keyEnd: U64 = keyStart + keyLen
      if keyEnd < length then
        if payload(keyEnd) == 34 then
          if Stigi.Parse.sliceEquals(length, payload, keyStart, keyEnd, keyLen, keyName) then
            val scan: U64 = Stigi.Server.advancePastColon(length, payload, keyEnd + 1)
            if scan < length && payload(scan) == 34 then
              val contentStart: U64 = scan + 1
              val foundSpan: Stigi.Server.EnvelopeOutcome = Stigi.Server.findClosingQuote(length, payload, contentStart)
              foundSpan match
                case EnvelopeInvalid => ()
                case EnvelopeSpan(spanStart, spanEnd) =>
                  if spanStart <= spanEnd && spanEnd <= length then
                    valStart = spanStart
                    valEnd = spanEnd
                    found = true
                  else
                    ()
            else
              ()
          else
            ()
        else
          ()
      else
        ()
    else
      ()
    cursor = cursor + 1
  if found then
    EnvelopeSpan(valStart, valEnd)
  else
    EnvelopeInvalid
