// prompt: the end of an identifier or number that starts at a position, after its last body byte.
def Stigi.Lex.identEnd(srcCap: Stigi.ByteCap, src: Array[U8](srcCap), srcLen: U64, pos: U64) <lessOrEqual(srcLen, srcCap), below(pos, srcLen)>: U64 <below(pos, result), lessOrEqual(result, srcLen)> =
  var endPos: U64 = pos + 1
  while endPos < srcLen && src(endPos) is 'a'..'z' | 'A'..'Z' | '0'..'9' | '_' do
    @<below(pos, endPos), lessOrEqual(endPos, srcLen)>
    endPos = endPos + 1
  endPos
