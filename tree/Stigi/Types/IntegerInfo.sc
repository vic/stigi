// prompt: exact inclusive 128-bit integer bounds and closed bit width of a primitive type.
pub enum Stigi.Types.IntegerInfo:
  case IntSpec(low: Stigi.Exact.IntWide, high: Stigi.Exact.IntWide, width: Stigi.Types.IntWidth)
  case NonInteger
