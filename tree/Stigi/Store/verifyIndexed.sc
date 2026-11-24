// prompt: check the unit named by an index name span through the self-hosted door.
def Stigi.Store.verifyIndexed {region: Stigi.Region, Stigi.Mut} (dirCap: Stigi.ByteCap, dirBuf: Array[U8](dirCap), dirStart: U64, dirEnd: U64, scratch: Stigi.Store.Scratch, nameStart: U64, nameEnd: U64) <lessOrEqual(dirStart, dirEnd), lessOrEqual(dirEnd, dirCap), lessOrEqual(nameStart, nameEnd), lessOrEqual(nameEnd, 262144)>: Stigi.RunOutcome =
  val Scratch(pathBuf, indexBuf, unitBuf, toks, nodes, facts, rows, nextInRel, smallSlots) = scratch
  val Found(unitPathLen) = Stigi.Store.buildUnitPath(dirCap, dirBuf, dirStart, dirEnd, 262144, indexBuf, nameStart, nameEnd, pathBuf) else
    case Missing => RunFileError
  if unitPathLen < 4096 then
    val LoadedBytes(srcLen) = Stigi.Posix.readFileBytes{region}(unitPathLen, pathBuf, 65536, unitBuf, 0) else
      case LoadFailed => RunFileError
      case LoadOverflow => RunFileError
    if srcLen <= 65536 then
      Stigi.Door.verifyUnit{region}(65536, unitBuf, srcLen, 16384, toks, 16384, nodes, 16384, facts, 16384, rows, nextInRel, smallSlots) match
        case DoorAccepted(_, _, _) => RunSuccess
        case DoorGateFailed | DoorLexFailed | DoorParseFailed | DoorDatalogRejected(_) => RunDoorRejected
        case DoorOutOfMemory => RunOutOfMemory
    else RunFileError
  else RunFileError
