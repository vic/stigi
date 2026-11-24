// prompt: write the bytes of a literal at the printer position.
def Stigi.Syntax.Print.putLiteral {Stigi.Mut} (tables: Stigi.Syntax.Tables, pen: Stigi.Syntax.Print.Pen, size: U64, text: Array[U8](size)) <lessOrEqual(size, Stigi.Limit.maxUnitBytes)>: Stigi.Syntax.Print.Pen =
  pen match
    case Pen(at) =>
      val Tables(_, _, _, _, _, output) = tables
      if Stigi.Limit.maxUnitBytes - at < size then PenOverflow else Pen(Stigi.Array.copy[U8](size, text, 0, size, Stigi.Limit.maxUnitBytes, output, at))
    case PenOverflow => PenOverflow
    case PenFailed(fault) => PenFailed(fault)
