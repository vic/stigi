// prompt: check whether one token slice is a language keyword rejected by L0.
def Stigi.Parse.isRejectedKeyword(sourceLength: U64, source: Array[U8](sourceLength), startOffset: U64, endOffset: U64) <lessOrEqual(startOffset, endOffset), lessOrEqual(endOffset, sourceLength), below(sourceLength, 0x100000000)>: Bool =
  val groupOne: Bool = Stigi.Parse.sliceEquals(sourceLength, source, startOffset, endOffset, 6, "import") || Stigi.Parse.sliceEquals(sourceLength, source, startOffset, endOffset, 5, "class")
  val groupTwo: Bool = Stigi.Parse.sliceEquals(sourceLength, source, startOffset, endOffset, 6, "object") || Stigi.Parse.sliceEquals(sourceLength, source, startOffset, endOffset, 5, "trait")
  val groupThree: Bool = Stigi.Parse.sliceEquals(sourceLength, source, startOffset, endOffset, 6, "return") || Stigi.Parse.sliceEquals(sourceLength, source, startOffset, endOffset, 5, "throw")
  val groupFour: Bool = Stigi.Parse.sliceEquals(sourceLength, source, startOffset, endOffset, 3, "try") || Stigi.Parse.sliceEquals(sourceLength, source, startOffset, endOffset, 3, "let")
  groupOne || groupTwo || groupThree || groupFour
