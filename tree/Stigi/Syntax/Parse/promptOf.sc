// prompt: the prompt text after the prompt marker on the first line of the unit source, or a unit without it.
def Stigi.Syntax.Parse.promptOf(tables: Stigi.Syntax.Tables, size: U64) <lessOrEqual(size, Stigi.Limit.maxUnitBytes)>: Stigi.Syntax.Parse.Prompt =
  val Tables(source, _, _, _, _, _) = tables
  if size < 11 then PromptAbsent else if !Stigi.Lex.sliceEquals(Stigi.Limit.maxUnitBytes, source, 0, 11, 11, "// prompt: ") then PromptAbsent else
    var end: U64 = 11
    while end < size && source(end) != '\n' do
      @<lessOrEqual(end, size)>
      end = end + 1
    if 11 <= end && end < size then PromptAt(Text(11, end)) else PromptAbsent
