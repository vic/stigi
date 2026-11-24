// prompt: a scan over row indexes from an index to a stop, with the current row total.
enum Stigi.Datalog.Scan:
  case Scan(index: U64, stop: U64, total: U64)
