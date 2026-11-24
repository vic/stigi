// prompt: write a run of source bytes of the unit at the printer position.
def Stigi.Syntax.Print.putText {Stigi.Mut} (tables: Stigi.Syntax.Tables, pen: Stigi.Syntax.Print.Pen, text: Stigi.Syntax.Text) <below(0, Stigi.Limit.maxUnitBytes)>: Stigi.Syntax.Print.Pen =
  val Text(first, end) = text
  pen match
    case Pen(at) =>
      val Tables(source, _, _, _, _, output) = tables
      if Stigi.Limit.maxUnitBytes - at < end - first then PenOverflow else Pen(Stigi.Array.copy[U8](Stigi.Limit.maxUnitBytes, source, first, end, Stigi.Limit.maxUnitBytes, output, at))
    case PenOverflow => PenOverflow
    case PenFailed(fault) => PenFailed(fault)
