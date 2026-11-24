// prompt: a bounded slice naming one unit inside the names pool.
enum Stigi.Store.NameSlice:
  case NameSlice(pool: Array[U8](Stigi.Limit.maxIndexBytes), first: U64, endPos: U64) <lessOrEqual(first, endPos), lessOrEqual(endPos, Stigi.Limit.maxIndexBytes)>
