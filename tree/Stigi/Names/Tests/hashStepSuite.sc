// prompt: verify the names hash against the published FNV-1a 64-bit test vectors of the empty text, the text a and the text foobar, through the text hash and through one hash step.
val Stigi.Names.Tests.hashStepSuite {Stigi.Test}: Stigi.Test.Result =
  val empty: Array[U8](0) = ""
  val single: Array[U8](1) = "a"
  val word: Array[U8](6) = "foobar"
  val emptyHash: U64 = Stigi.Names.textHash(0, empty, 0, 0, false, Stigi.Names.hashSeed, 1)
  val singleHash: U64 = Stigi.Names.textHash(1, single, 0, 1, false, Stigi.Names.hashSeed, 2)
  val stepHash: U64 = Stigi.Names.hashStep(Stigi.Names.hashSeed, 97)
  val wordHash: U64 = Stigi.Names.textHash(6, word, 0, 6, false, Stigi.Names.hashSeed, 7)
  if emptyHash != 0xcbf29ce484222325 then Stigi.Test.expect(emptyHash, 0xcbf29ce484222325, "clause the hash of the empty text is the published vector 0xcbf29ce484222325") else if singleHash != 0xaf63dc4c8601ec8c then Stigi.Test.expect(singleHash, 0xaf63dc4c8601ec8c, "clause the text hash of a is the published vector 0xaf63dc4c8601ec8c") else if stepHash != 0xaf63dc4c8601ec8c then Stigi.Test.expect(stepHash, 0xaf63dc4c8601ec8c, "clause one hash step of the seed with the byte of a is the published vector 0xaf63dc4c8601ec8c") else Stigi.Test.expect(wordHash, 0x85944171f73967e8, "the text hash of foobar is the published vector 0x85944171f73967e8")
