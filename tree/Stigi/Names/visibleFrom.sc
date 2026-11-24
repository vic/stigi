// prompt: whether a unit may be named from another unit: it is public, or the other unit lies in its namespace.
def Stigi.Names.visibleFrom(image: Stigi.Names.Image, target: U64, from: U64) <below(from, Stigi.Limit.maxUnits)>: Bool =
  image match
    case Image(_, entries, count, _) =>
      if target < count && from < count then
        entries(target) match
          case Entry(_, _, _, isPublic) => isPublic || Stigi.Names.sameNamespace(image, target, from)
      else false
