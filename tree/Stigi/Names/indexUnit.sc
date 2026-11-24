// prompt: index one unit: parse its source, copy its name into the name pool at the running pool end and check the copy reached the new end, check the name equals the name the unit declares, add it to the image and record its members, or say why it failed; after a failure the image is discarded.
def Stigi.Names.indexUnit {Stigi.Mut} (image: Stigi.Names.Image, members: Stigi.Names.Members, tables: Stigi.Syntax.Tables, poolEnd: U64, unit: Stigi.Names.UnitSource) <lessOrEqual(poolEnd, Stigi.Limit.maxIndexBytes)>: Stigi.Names.IndexOutcome =
  val UnitSource(name, nameSize, source, size) = unit
  Stigi.Syntax.Parse.parseUnit(tables, Stigi.Limit.maxUnitBytes, source, size) match
    case ParseRejected(_, _) | LexRejected(_) => IndexFailed(SourceRejected)
    case Parsed(root, _, _) =>
      val shape: Stigi.Names.UnitShape = Stigi.Names.shapeOfUnit(tables, root)
      Stigi.Names.declOf(tables, root) match
        case DeclMissing => IndexFailed(SourceRejected)
        case DeclFound(declNode) =>
          val Image(pool, _, count, _) = image
          if nameSize <= Stigi.Limit.maxIndexBytes - poolEnd && count < Stigi.Limit.maxUnits then
            val newEnd: U64 = poolEnd + nameSize
            val ended: U64 = Stigi.Array.copy[U8](Stigi.Limit.maxUnitBytes, name, 0, nameSize, Stigi.Limit.maxIndexBytes, pool, poolEnd)
            if ended == newEnd then
              Stigi.Names.rootDeclaredName(tables, root) match
                case Nameless => IndexFailed(NameMismatch)
                case NameFound(Text(first, end)) =>
                  if end - first == nameSize && Stigi.Names.poolHasText(pool, poolEnd, tables, first, end, nameSize + 1) then
                    Stigi.Names.addEntry(image, poolEnd, newEnd, shape) match
                      case ImageFull => IndexFailed(ImageExhausted)
                      case UnitUnshaped => IndexFailed(SourceRejected)
                      case Added(grown) =>
                        Stigi.Names.recordMembers(members, tables, count, declNode) match
                          case MembersFull => IndexFailed(ImageExhausted)
                          case MembersMalformed => IndexFailed(SourceRejected)
                          case Recorded(recorded) => Indexed(grown, recorded, newEnd)
                  else IndexFailed(NameMismatch)
            else IndexFailed(ImageExhausted)
          else IndexFailed(ImageExhausted)
