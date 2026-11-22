// prompt: write all thirty-two little-endian bytes of a BLAKE3 digest into an output buffer.
pub def Stigi.Hash.writeDigestBytes {Stigi.Mut} (digest: Stigi.Hash.Digest, outCap: U64, output: Array[U8](outCap), offset: U64) <lessOrEqual(offset, outCap), below(outCap, 0x100000000)>: U64 <lessOrEqual(offset, result), lessOrEqual(result, outCap)> =
  val DigestWords(wordZero, wordOne, wordTwo, wordThree, wordFour, wordFive, wordSix, wordSeven) = digest
  val stepOne: U64 = Stigi.Hash.writeLittleWord(wordZero, outCap, output, offset)
  val stepTwo: U64 = Stigi.Hash.writeLittleWord(wordOne, outCap, output, stepOne)
  val stepThree: U64 = Stigi.Hash.writeLittleWord(wordTwo, outCap, output, stepTwo)
  val stepFour: U64 = Stigi.Hash.writeLittleWord(wordThree, outCap, output, stepThree)
  val stepFive: U64 = Stigi.Hash.writeLittleWord(wordFour, outCap, output, stepFour)
  val stepSix: U64 = Stigi.Hash.writeLittleWord(wordFive, outCap, output, stepFive)
  val stepSeven: U64 = Stigi.Hash.writeLittleWord(wordSix, outCap, output, stepSix)
  Stigi.Hash.writeLittleWord(wordSeven, outCap, output, stepSeven)
