// prompt: thirty-two byte BLAKE3 digest represented as eight little-endian U32 words.
pub enum Stigi.Hash.Digest:
  case DigestWords(wordZero: U32, wordOne: U32, wordTwo: U32, wordThree: U32, wordFour: U32, wordFive: U32, wordSix: U32, wordSeven: U32)
