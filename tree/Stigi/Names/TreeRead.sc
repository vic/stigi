// prompt: what one tree pass reads: the tree directory path, the reusable buffers, the parse tables and the length of the index text.
enum Stigi.Names.TreeRead:
  case TreeRead(dirCap: Stigi.ByteCap, dirBuf: Array[U8](dirCap), dirStart: U64, dirEnd: U64, scratch: Stigi.Names.TreeScratch, tables: Stigi.Syntax.Tables, indexLen: U64) <lessOrEqual(dirStart, dirEnd), lessOrEqual(dirEnd, dirCap), lessOrEqual(indexLen, Stigi.Limit.maxIndexBytes)>
