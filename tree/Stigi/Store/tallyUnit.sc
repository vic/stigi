// prompt: add the outcome of one unit to the running counts of a parse check, reporting a unit that did not print back.
def Stigi.Store.tallyUnit {region: Stigi.Region, Stigi.Mut} (tally: Stigi.Store.ParseTally, outcome: Stigi.Syntax.RoundTrip, scratch: Stigi.Store.ParseScratch, nameStart: U64, nameEnd: U64) <lessOrEqual(nameStart, nameEnd), lessOrEqual(nameEnd, 65536)>: Stigi.Store.ParseTally =
  val ParseTally(units, matched, bytes, tokens, nodes, reportAt) = tally
  val counted: U64 = Stigi.Store.addCount(units, 1)
  outcome match
    case TextMatched(size, tokenCount, nodeCount) => ParseTally(counted, Stigi.Store.addCount(matched, 1), Stigi.Store.addCount(bytes, size), Stigi.Store.addCount(tokens, tokenCount), Stigi.Store.addCount(nodes, nodeCount), reportAt)
    case TextMismatched(_) | UnitLexFailed(_) | UnitParseFailed(_, _) | UnitPrintFailed | UnitUnreadable => ParseTally(counted, matched, bytes, tokens, nodes, Stigi.Store.putFailure{region}(scratch, reportAt, nameStart, nameEnd, outcome))
