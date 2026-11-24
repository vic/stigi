// prompt: what a handler member lookup finds: the pool run of the capability name a handler stands for, or no such member.
enum Stigi.Names.ForRead:
  case ForText(first: U64, end: U64) <lessOrEqual(first, end), lessOrEqual(end, Stigi.Limit.maxIndexBytes)>
  case MissingFor
