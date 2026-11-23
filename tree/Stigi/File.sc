// prompt: typestate file capability ensuring every opened handle closes once.
capability Stigi.File:
  state Open, Closed
  start Open
  stop Closed
  def readChunk {Stigi.Mut} (count: U64, buf: Array[U8](count)) <at(self, Open), below(count, Stigi.Limit.maxSourceBytes)>: Stigi.File.ReadResult <at(self, Open)>
  def writeChunk(count: U64, buf: Array[U8](count)) <at(self, Open), below(count, Stigi.Limit.maxSourceBytes)>: Stigi.File.WriteResult <at(self, Open)>
  def fileSize() <at(self, Open)>: Stigi.File.SizeResult <at(self, Open)>
  def closeFile() <at(self, Open)>: Stigi.File.CloseResult <at(self, Closed)>
