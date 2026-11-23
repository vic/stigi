// prompt: write one line of ASCII bytes followed by a newline through Stigi.Emit.Writer.
def Stigi.Emit.writeLine {writer: Stigi.Emit.Writer, Stigi.Mut} (outCap: Stigi.EmitCap, outBuf: Array[U8](outCap), litLen: Stigi.ByteCap, text: Array[U8](litLen)): Unit =
  writer.putSlice(outCap, outBuf, litLen, text, 0, litLen)
  writer.putByte(outCap, outBuf, 10)
