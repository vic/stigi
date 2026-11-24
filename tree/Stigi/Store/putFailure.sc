// prompt: write one report line for a unit that did not print back byte for byte: its name, the failure and its position.
def Stigi.Store.putFailure {region: Stigi.Region, Stigi.Mut} (scratch: Stigi.Store.ParseScratch, at: U64, nameStart: U64, nameEnd: U64, outcome: Stigi.Syntax.RoundTrip) <lessOrEqual(at, 65536), lessOrEqual(nameStart, nameEnd), lessOrEqual(nameEnd, 262144)>: U64 <lessOrEqual(result, 65536)> =
  val ParseScratch(_, indexBuf, _, report, _) = scratch
  val named: U64 = Stigi.Array.copy[U8](262144, indexBuf, nameStart, nameEnd, 65536, report, at)
  val spaced: U64 = Stigi.Array.copy[U8](1, " ", 0, 1, 65536, report, named)
  val worded: U64 = outcome match
    case TextMatched(_, _, _) => spaced
    case TextMismatched(_) => Stigi.Array.copy[U8](8, "mismatch", 0, 8, 65536, report, spaced)
    case UnitLexFailed(_) => Stigi.Array.copy[U8](3, "lex", 0, 3, 65536, report, spaced)
    case UnitParseFailed(UnexpectedToken, _) => Stigi.Array.copy[U8](10, "unexpected", 0, 10, 65536, report, spaced)
    case UnitParseFailed(TooDeep, _) => Stigi.Array.copy[U8](4, "deep", 0, 4, 65536, report, spaced)
    case UnitParseFailed(UnitTooLarge, _) => Stigi.Array.copy[U8](5, "large", 0, 5, 65536, report, spaced)
    case UnitParseFailed(ChildAfterParent, _) => Stigi.Array.copy[U8](5, "order", 0, 5, 65536, report, spaced)
    case UnitPrintFailed => Stigi.Array.copy[U8](5, "print", 0, 5, 65536, report, spaced)
    case UnitUnreadable => Stigi.Array.copy[U8](10, "unreadable", 0, 10, 65536, report, spaced)
  val position: U64 = outcome match
    case TextMismatched(printed) => printed
    case UnitLexFailed(offset) => offset
    case UnitParseFailed(_, offset) => offset
    case TextMatched(_, _, _) | UnitPrintFailed | UnitUnreadable => 0
  val numbered: U64 = Stigi.Store.putNumber{region}(report, Stigi.Array.copy[U8](1, " ", 0, 1, 65536, report, worded), position)
  Stigi.Array.copy[U8](1, "\n", 0, 1, 65536, report, numbered)
