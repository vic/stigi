// prompt: whether the unit named by the first index lies in the namespace of the unit named by the second, or is that very unit when it has no namespace.
def Stigi.Names.sameNamespace(image: Stigi.Names.Image, target: U64, from: U64) <below(from, Stigi.Limit.maxUnits)>: Bool =
  image match
    case Image(pool, entries, count, _) =>
      if target < count && from < count then
        entries(target) match
          case Entry(targetFirst, targetEnd, _, _) =>
            val spaceEnd: U64 = Stigi.Names.namespaceEnd(pool, targetFirst, targetEnd, targetEnd - targetFirst + 1)
            if spaceEnd <= targetFirst then target == from else Stigi.Names.nameUnder(pool, entries(from), targetFirst, spaceEnd)
      else false
