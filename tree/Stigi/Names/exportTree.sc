// prompt: export the names of a tree: index every unit named on the index into the image, then walk every unit for its edges, and count both, or say which unit and step failed.
pub def Stigi.Names.exportTree {region: Stigi.Region, Stigi.Mut} (dirCap: Stigi.ByteCap, dirBuf: Array[U8](dirCap), dirStart: U64, dirEnd: U64) <lessOrEqual(dirStart, dirEnd), lessOrEqual(dirEnd, dirCap)>: Stigi.Names.ExportOutcome =
  val Allocated(tables) = Stigi.Syntax.allocate{region}() else
    case AllocationFailed => ExportFailed(BufferUnavailable)
  Stigi.Names.exportTreeWithTables{region}(dirCap, dirBuf, dirStart, dirEnd, tables)
