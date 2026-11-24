// prompt: read the unit file named by an index name span into the source buffer and copy the name into the name buffer.
def Stigi.Names.loadTreeUnit {region: Stigi.Region, Stigi.Mut} (dirCap: Stigi.ByteCap, dirBuf: Array[U8](dirCap), dirStart: U64, dirEnd: U64, scratch: Stigi.Names.TreeScratch, nameStart: U64, nameEnd: U64) <lessOrEqual(dirStart, dirEnd), lessOrEqual(dirEnd, dirCap), lessOrEqual(nameStart, nameEnd), lessOrEqual(nameEnd, Stigi.Limit.maxIndexBytes)>: Stigi.Names.UnitLoad =
  val TreeScratch(pathBuf, indexBuf, nameBuf, sourceBuf, _, _) = scratch
  val Found(unitPathLen) = Stigi.Store.buildUnitPath(dirCap, dirBuf, dirStart, dirEnd, Stigi.Limit.maxIndexBytes, indexBuf, nameStart, nameEnd, pathBuf) else
    case Missing => UnitMissing
  if unitPathLen < Stigi.Limit.maxPathBytes then
    val nameSize: U64 = Stigi.Array.copy[U8](Stigi.Limit.maxIndexBytes, indexBuf, nameStart, nameEnd, Stigi.Limit.maxUnitBytes, nameBuf, 0)
    if nameSize == nameEnd - nameStart then
      val LoadedBytes(size) = Stigi.Posix.readFileBytes{region}(unitPathLen, pathBuf, Stigi.Limit.maxUnitBytes, sourceBuf, 0) else
        case LoadFailed => UnitMissing
        case LoadOverflow => UnitMissing
      if size <= Stigi.Limit.maxUnitBytes then UnitRead(nameSize, size) else UnitMissing
    else UnitMissing
  else UnitMissing
