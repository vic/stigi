// prompt: current byte write cursors inside the typeRange and typeWidth fact buffers.
enum Stigi.Store.TypeRowCursors:
  case TypeCursors(rangeEnd: U64, widthEnd: U64) <lessOrEqual(rangeEnd, 4096), lessOrEqual(widthEnd, 4096)>
