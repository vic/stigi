// prompt: collect dotted qualified references from a unit body into parallel start and end offset arrays.
pub def Stigi.Parse.collectReferences {Stigi.Mut} (length: U64, source: Array[U8](length), bodyStart: U64, maxRefs: U64, refStarts: Array[U64](maxRefs), refEnds: Array[U64](maxRefs)) <lessOrEqual(bodyStart, length), below(length, 0x100000000)>: U64 <lessOrEqual(result, maxRefs)> =
  var cursor: U64 = bodyStart
  var stepIndex: U64 = 0
  var refCount: U64 = 0
  var done: Bool = false
  while cursor < length && stepIndex < length && !done do
    @<lessOrEqual(cursor, length), lessOrEqual(stepIndex, length), lessOrEqual(refCount, maxRefs), decreases(length - stepIndex)>
    val scan: Stigi.Lex.ScanOutcome = Stigi.Lex.nextToken(length, source, cursor)
    scan match
      case ScanToken(tokKind, tokStart, tokEnd) =>
        if tokEnd > cursor && tokEnd <= length then
          cursor = tokEnd
        else
          cursor = length
        stepIndex = stepIndex + 1
        tokKind match
          case TokenEnd =>
            done = true
          case TokenUpperIdent =>
            if tokStart <= tokEnd && tokEnd <= length && refCount < maxRefs then
              if Stigi.Parse.hasDotSeparator(length, source, tokStart, tokEnd) then
                refStarts(refCount) := tokStart
                refEnds(refCount) := tokEnd
                refCount = refCount + 1
              else
                done = false
            else
              done = false
          case TokenIdent =>
            done = false
          case TokenNewline =>
            done = false
          case TokenInt =>
            done = false
          case TokenByte =>
            done = false
          case TokenText =>
            done = false
          case TokenComment =>
            done = false
          case TokenPrompt =>
            done = false
          case TokenPunct =>
            done = false
      case ScanError(_, _) =>
        done = true
        stepIndex = stepIndex + 1
  refCount
