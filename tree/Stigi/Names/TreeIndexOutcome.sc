// prompt: the outcome of indexing a tree from a line position: the grown image and members with the new end of the name pool, or the unit index and name size of the unit that stopped it and why.
enum Stigi.Names.TreeIndexOutcome:
  case TreeIndexed(image: Stigi.Names.Image, members: Stigi.Names.Members, newEnd: U64) <lessOrEqual(newEnd, Stigi.Limit.maxIndexBytes)>
  case TreeIndexStopped(entry: U64, nameSize: U64, why: Stigi.Names.TreeFailure) <lessOrEqual(nameSize, Stigi.Limit.maxUnitBytes)>
