// prompt: pair of directory slices for reading a tree and writing exported type fact files.
enum Stigi.Store.TypesTarget:
  case TypesTarget(dirCap: Stigi.ByteCap, dirBuf: Array[U8](dirCap), dirStart: U64, dirEnd: U64, outStart: U64, outEnd: U64) <lessOrEqual(dirStart, dirEnd), lessOrEqual(dirEnd, dirCap), lessOrEqual(outStart, outEnd), lessOrEqual(outEnd, dirCap)>
