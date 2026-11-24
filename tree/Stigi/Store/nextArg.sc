// prompt: the NUL-terminated argument of a command line that starts at a position.
def Stigi.Store.nextArg(bufCap: Stigi.ByteCap, buf: Array[U8](bufCap), bufLen: U64, pos: U64) <lessOrEqual(bufLen, bufCap)>: Stigi.Store.ArgSpan =
  if pos < bufLen then
    var endPos: U64 = pos
    while endPos < bufLen && buf(endPos) != '\0' do
      @<lessOrEqual(pos, endPos), lessOrEqual(endPos, bufLen)>
      endPos = endPos + 1
    ArgAt(pos, endPos, if endPos < bufLen then endPos + 1 else endPos)
  else ArgsEnd
