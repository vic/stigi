// prompt: the reusable buffers of one tree pass: the path, the index text, one unit name, one unit source, and the walk task and local stacks.
enum Stigi.Names.TreeScratch:
  case TreeScratch(pathBuf: Array[U8](Stigi.Limit.maxPathBytes), indexBuf: Array[U8](Stigi.Limit.maxIndexBytes), nameBuf: Array[U8](Stigi.Limit.maxUnitBytes), sourceBuf: Array[U8](Stigi.Limit.maxUnitBytes), tasks: Array[Stigi.Names.Task](Stigi.Names.maxScope), locals: Array[Stigi.Names.Local](Stigi.Names.maxScope))
