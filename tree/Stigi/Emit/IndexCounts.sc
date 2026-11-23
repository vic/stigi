// prompt: single-case return bundle carrying updated symbol count, field count, and token cursor.
enum Stigi.Emit.IndexCounts:
  case IndexCounts(symCount: U64, fieldCount: U64, nextTok: U64)
