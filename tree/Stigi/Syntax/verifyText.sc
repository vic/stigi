// prompt: parse the first bytes of a text buffer as one unit, print its syntax table, and compare the printed bytes with the source bytes.
pub def Stigi.Syntax.verifyText {Stigi.Mut} (tables: Stigi.Syntax.Tables, textCap: U64, text: Array[U8](textCap), size: U64) <lessOrEqual(size, textCap), lessOrEqual(size, Stigi.Limit.maxUnitBytes)>: Stigi.Syntax.RoundTrip =
  Stigi.Syntax.Parse.parseUnit(tables, textCap, text, size) match
    case LexRejected(offset) => UnitLexFailed(offset)
    case ParseRejected(fault, token) =>
      val Token(_, startPos, _, _, _) = Stigi.Syntax.Parse.tokenAt(tables, token)
      UnitParseFailed(fault, startPos)
    case Parsed(root, nodes, tokens) =>
      Stigi.Syntax.Print.printNode(tables, root, Pen(0), 0, Stigi.Limit.maxParseDepth) match
        case PenOverflow | PenFailed(_) => UnitPrintFailed
        case Pen(written) => if written == size && Stigi.Syntax.sameText(tables, 0, size, size) then TextMatched(size, tokens, nodes) else TextMismatched(written)
