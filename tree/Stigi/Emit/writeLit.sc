// prompt: write a fixed byte literal into the emitter output buffer via Stigi.Emit.Writer.
def Stigi.Emit.writeLit {writer: Stigi.Emit.Writer, Stigi.Mut} (outCap: Stigi.EmitCap, outBuf: Array[U8](outCap), litLen: Stigi.ByteCap, text: Array[U8](litLen)): Unit =
  writer.putSlice(outCap, outBuf, litLen, text, 0, litLen)
