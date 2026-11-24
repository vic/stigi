// prompt: the running counts of a parse check: units, units printed back byte for byte, their bytes, tokens and nodes, and the end of the report text.
enum Stigi.Store.ParseTally:
  case ParseTally(units: U64, matched: U64, bytes: U64, tokens: U64, nodes: U64, reportAt: U64) <lessOrEqual(reportAt, 65536)>
