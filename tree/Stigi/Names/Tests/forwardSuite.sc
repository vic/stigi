// prompt: verify that a parameter is bound before the types of all parameters are visited, for an extern def and for a plain def as the seed does, and that a name declared nowhere stays unresolved.
val Stigi.Names.Tests.forwardSuite {Stigi.Test}: Stigi.Test.Result =
  val externOpen: U64 = Stigi.Names.Tests.unresolvedOf{Stigi.Region}(94, "// prompt: demo.\npub extern \"c\" def A.read(desc: U64, buf: Array[U8](count), count: U64): U64\n", 94)
  val plainOpen: U64 = Stigi.Names.Tests.unresolvedOf{Stigi.Region}(89, "// prompt: demo.\ndef A.read(desc: U64, buf: Array[U8](count), count: U64): U64 =\n  count\n", 89)
  val ghostOpen: U64 = Stigi.Names.Tests.unresolvedOf{Stigi.Region}(89, "// prompt: demo.\ndef A.read(desc: U64, buf: Array[U8](ghost), count: U64): U64 =\n  count\n", 89)
  if externOpen == 0 then
    if plainOpen == 0 then
      if ghostOpen == 1 then Stigi.Test.expect(true, true, "a following parameter is visible in an earlier type of the same head and a name declared nowhere stays unresolved") else Stigi.Test.expect(false, true, "clause an undeclared name stays unresolved")
    else Stigi.Test.expect(false, true, "clause a plain def resolves the forward parameter")
  else Stigi.Test.expect(false, true, "clause an extern def resolves the forward parameter")
