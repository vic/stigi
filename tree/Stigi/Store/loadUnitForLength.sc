// prompt: read and parse one unit from a tree directory into the length scratch source and syntax tables.
def Stigi.Store.loadUnitForLength {region: Stigi.Region, Stigi.Mut} (target: Stigi.Store.TypesTarget, scratch: Stigi.Store.LengthScratch, image: Stigi.Names.Image, unitIdx: U64) <below(unitIdx, Stigi.Limit.maxUnits)>: Stigi.Syntax.Parse.Outcome =
  target match
    case TypesTarget(dirCap, dirBuf, dirStart, dirEnd, _, _) =>
      val Image(namePool, entries, count, _) = image
      if count <= unitIdx then ParseRejected(UnexpectedToken, 0) else
        val Entry(nameFirst, nameEnd, _, _) = entries(unitIdx)
        scratch match
          case LengthScratch(pathBuf, sourceBuf, synTables, _, _, _, _, _, _, _, _, _, _, _, _, _) =>
            Stigi.Store.buildUnitPath(dirCap, dirBuf, dirStart, dirEnd, Stigi.Limit.maxIndexBytes, namePool, nameFirst, nameEnd, pathBuf) match
              case Missing => ParseRejected(UnexpectedToken, 0)
              case Found(pathLen) =>
                if pathLen < 4096 then
                  Stigi.Posix.readFileBytes{region}(pathLen, pathBuf, Stigi.Limit.maxUnitBytes, sourceBuf, 0) match
                    case LoadFailed | LoadOverflow => ParseRejected(UnexpectedToken, 0)
                    case LoadedBytes(fileLen) =>
                      if fileLen <= Stigi.Limit.maxUnitBytes then
                        Stigi.Syntax.Parse.parseUnit(synTables, Stigi.Limit.maxUnitBytes, sourceBuf, fileLen)
                      else ParseRejected(UnexpectedToken, 0)
                else ParseRejected(UnexpectedToken, 0)
