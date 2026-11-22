// prompt: run the unit-level tenet checks for prompt header and forbidden tab characters.
pub def Stigi.TenetGate.checkUnitSource(length: U64, source: Array[U8](length)) <lessOrEqual(0, length)>: Stigi.TenetGate.Finding =
  if !Stigi.TenetGate.hasPromptLine(length, source) then
    Stigi.TenetGate.Finding.FindingPromptMissing
  else
    var cursor: U64 = 0
    var finding: Stigi.TenetGate.Finding = Stigi.TenetGate.Finding.FindingClean
    var failed: Bool = false
    while cursor < length && !failed do
      @<lessOrEqual(cursor, length), decreases(length - cursor)>
      if source(cursor) == 9 then
        finding = Stigi.TenetGate.Finding.FindingTabIndent(cursor)
        failed = true
        cursor = cursor + 1
      else
        cursor = cursor + 1
    finding
