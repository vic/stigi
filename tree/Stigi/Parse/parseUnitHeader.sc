// prompt: parse the leading visibility, declaration kind, and dotted name of one L0 unit.
pub def Stigi.Parse.parseUnitHeader(length: U64, source: Array[U8](length)) <below(length, 0x100000000)>: Stigi.Parse.UnitHeaderOutcome =
  var cursor: U64 = 0
  var stepIndex: U64 = 0
  var seenPublic: Bool = false
  var kind: Stigi.Parse.DeclKind = Stigi.Parse.DeclKind.DeclUnknown
  var awaitingName: Bool = false
  var done: Bool = false
  var outcome: Stigi.Parse.UnitHeaderOutcome = Stigi.Parse.UnitHeaderOutcome.UnitHeaderError(0, 1)
  while cursor < length && stepIndex < length && !done do
    @<lessOrEqual(cursor, length), lessOrEqual(stepIndex, length), decreases(length - stepIndex)>
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
          case TokenNewline =>
            done = false
          case TokenComment =>
            done = false
          case TokenPrompt =>
            done = false
          case TokenIdent =>
            if tokStart <= tokEnd && tokEnd <= length then
              if awaitingName then
                val isModKeyword: Bool = Stigi.Parse.sliceEquals(length, source, tokStart, tokEnd, 3, "def") || Stigi.Parse.sliceEquals(length, source, tokStart, tokEnd, 3, "val")
                if isModKeyword then
                  done = false
                else
                  outcome = Stigi.Parse.UnitHeaderOutcome.UnitHeader(kind, seenPublic, tokStart, tokEnd)
                  done = true
              else
                if Stigi.Parse.isRejectedKeyword(length, source, tokStart, tokEnd) then
                  outcome = Stigi.Parse.UnitHeaderOutcome.UnitHeaderError(tokStart, 2)
                  done = true
                else
                  if Stigi.Parse.sliceEquals(length, source, tokStart, tokEnd, 3, "pub") then
                    seenPublic = true
                  else
                    val parsedKind: Stigi.Parse.DeclKind = Stigi.Parse.classifyKeyword(length, source, tokStart, tokEnd)
                    parsedKind match
                      case DeclUnknown =>
                        done = false
                      case DeclDef =>
                        kind = parsedKind
                        awaitingName = true
                      case DeclLemma =>
                        kind = parsedKind
                        awaitingName = true
                      case DeclVal =>
                        kind = parsedKind
                        awaitingName = true
                      case DeclEnum =>
                        kind = parsedKind
                        awaitingName = true
                      case DeclType =>
                        kind = parsedKind
                        awaitingName = true
                      case DeclCapability =>
                        kind = parsedKind
                        awaitingName = true
                      case DeclHandler =>
                        kind = parsedKind
                        awaitingName = true
                      case DeclExtern =>
                        kind = parsedKind
                        awaitingName = true
                      case DeclRelation =>
                        kind = parsedKind
                        awaitingName = true
                      case DeclRule =>
                        kind = parsedKind
                        awaitingName = true
            else
              done = true
          case TokenUpperIdent =>
            if awaitingName && tokStart <= tokEnd && tokEnd <= length then
              outcome = Stigi.Parse.UnitHeaderOutcome.UnitHeader(kind, seenPublic, tokStart, tokEnd)
              done = true
            else
              done = false
          case TokenInt =>
            done = false
          case TokenByte =>
            done = false
          case TokenText =>
            done = false
          case TokenPunct =>
            done = false
      case ScanError(offset, reasonCode) =>
        outcome = Stigi.Parse.UnitHeaderOutcome.UnitHeaderError(offset, reasonCode)
        done = true
        stepIndex = stepIndex + 1
  outcome
