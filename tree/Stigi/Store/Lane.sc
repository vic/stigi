// prompt: one facts lane, its size, its bytes and the end of its rows, which never lies past the size.
enum Stigi.Store.Lane:
  case Lane(size: U64, bytes: Array[U8](size), end: U64) <lessOrEqual(end, size), below(size, Stigi.Limit.maxEmitBytes)>
