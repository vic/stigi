// prompt: closed outcome of building the path of a facts file: the path length that fits, or an overflow of the path capacity.
enum Stigi.Store.FactsPath:
  case PathBuilt(length: U64) <below(length, Stigi.Limit.maxPathBytes)>
  case PathOverflow
