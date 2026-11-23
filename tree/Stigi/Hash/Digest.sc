// prompt: thirty-two-byte BLAKE3 digest stored inline in eight thirty-two-bit words.
pub enum Stigi.Hash.Digest:
  case Digest(wordZero: U32, wordOne: U32, wordTwo: U32, wordThree: U32, wordFour: U32, wordFive: U32, wordSix: U32, wordSeven: U32)
