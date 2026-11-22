// prompt: classify a declaration keyword slice into its L0 declaration kind.
def Stigi.Parse.classifyKeyword(sourceLength: U64, source: Array[U8](sourceLength), startOffset: U64, endOffset: U64) <lessOrEqual(startOffset, endOffset), lessOrEqual(endOffset, sourceLength), below(sourceLength, 0x100000000)>: Stigi.Parse.DeclKind =
  if Stigi.Parse.sliceEquals(sourceLength, source, startOffset, endOffset, 3, "def") then
    Stigi.Parse.DeclKind.DeclDef
  else
    if Stigi.Parse.sliceEquals(sourceLength, source, startOffset, endOffset, 5, "lemma") then
      Stigi.Parse.DeclKind.DeclLemma
    else
      if Stigi.Parse.sliceEquals(sourceLength, source, startOffset, endOffset, 3, "val") then
        Stigi.Parse.DeclKind.DeclVal
      else
        if Stigi.Parse.sliceEquals(sourceLength, source, startOffset, endOffset, 4, "enum") then
          Stigi.Parse.DeclKind.DeclEnum
        else
          if Stigi.Parse.sliceEquals(sourceLength, source, startOffset, endOffset, 4, "type") then
            Stigi.Parse.DeclKind.DeclType
          else
            if Stigi.Parse.sliceEquals(sourceLength, source, startOffset, endOffset, 10, "capability") then
              Stigi.Parse.DeclKind.DeclCapability
            else
              if Stigi.Parse.sliceEquals(sourceLength, source, startOffset, endOffset, 7, "handler") then
                Stigi.Parse.DeclKind.DeclHandler
              else
                if Stigi.Parse.sliceEquals(sourceLength, source, startOffset, endOffset, 6, "extern") then
                  Stigi.Parse.DeclKind.DeclExtern
                else
                  if Stigi.Parse.sliceEquals(sourceLength, source, startOffset, endOffset, 8, "relation") then
                    Stigi.Parse.DeclKind.DeclRelation
                  else
                    if Stigi.Parse.sliceEquals(sourceLength, source, startOffset, endOffset, 4, "rule") then
                      Stigi.Parse.DeclKind.DeclRule
                    else
                      Stigi.Parse.DeclKind.DeclUnknown
