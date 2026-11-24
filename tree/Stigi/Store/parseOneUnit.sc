// prompt: load the unit named by an index name span and check that it parses and prints back byte for byte.
def Stigi.Store.parseOneUnit {region: Stigi.Region, Stigi.Mut} (dirCap: Stigi.ByteCap, dirBuf: Array[U8](dirCap), dirStart: U64, dirEnd: U64, scratch: Stigi.Store.ParseScratch, nameStart: U64, nameEnd: U64) <lessOrEqual(dirStart, dirEnd), lessOrEqual(dirEnd, dirCap), lessOrEqual(nameStart, nameEnd), lessOrEqual(nameEnd, 262144)>: Stigi.Syntax.RoundTrip =
  val ParseScratch(pathBuf, indexBuf, unitBuf, _, tables) = scratch
  val Found(unitPathLen) = Stigi.Store.buildUnitPath(dirCap, dirBuf, dirStart, dirEnd, 262144, indexBuf, nameStart, nameEnd, pathBuf) else
    case Missing => UnitUnreadable
  if unitPathLen < 4096 then
    val LoadedBytes(srcLen) = Stigi.Posix.readFileBytes{region}(unitPathLen, pathBuf, 65536, unitBuf, 0) else
      case LoadFailed => UnitUnreadable
      case LoadOverflow => UnitUnreadable
    if srcLen <= 65536 then Stigi.Syntax.verifyText(tables, 65536, unitBuf, srcLen) else UnitUnreadable
  else UnitUnreadable
