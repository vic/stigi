// prompt: the outcome of indexing one unit: the grown image and members with the new end of the name pool, or why it failed.
enum Stigi.Names.IndexOutcome:
  case Indexed(image: Stigi.Names.Image, members: Stigi.Names.Members, newEnd: U64) <lessOrEqual(newEnd, Stigi.Limit.maxIndexBytes)>
  case IndexFailed(why: Stigi.Names.TreeFailure)
