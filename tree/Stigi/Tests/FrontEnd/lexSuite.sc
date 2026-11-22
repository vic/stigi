// prompt: test lexical scanning of prompts, identifiers, numbers, literals, operators, and tabs.
val Stigi.Tests.FrontEnd.lexSuite {Stigi.Test}: Stigi.Test.Result =
  val sample: Array[U8](46) = "// prompt: demo\npub def Demo.sum(a: U64) = 0x5"
  val tokenCount: U64 = Stigi.Lex.countTokens(46, sample)
  val firstTok: Stigi.Lex.ScanOutcome = Stigi.Lex.nextToken(46, sample, 0)
  val firstValid: Bool = firstTok match
    case ScanToken(kind, startPos, endPos) =>
      kind match
        case TokenPrompt => startPos == 0 && endPos == 15
        case TokenEnd => false
        case TokenNewline => false
        case TokenIdent => false
        case TokenUpperIdent => false
        case TokenInt => false
        case TokenByte => false
        case TokenText => false
        case TokenComment => false
        case TokenPunct => false
    case ScanError(_, _) => false
  val tabSample: Array[U8](5) = "\tdef "
  val tabTok: Stigi.Lex.ScanOutcome = Stigi.Lex.nextToken(5, tabSample, 0)
  val tabRejected: Bool = tabTok match
    case ScanToken(_, _, _) => false
    case ScanError(off, code) => off == 0 && code == 1
  val asciiChecks: Bool = Stigi.Lex.isLetterByte(65) && Stigi.Lex.isDigitByte(57)
  Stigi.Lex.cursorWithinLength(0, 46)
  val allPassed: Bool = tokenCount == 11 && firstValid && tabRejected && asciiChecks
  Stigi.Test.expect(allPassed, true, "lexer scans prompt, eleven tokens, tab error, and ASCII bounds")
