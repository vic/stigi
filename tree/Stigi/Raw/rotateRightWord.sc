// prompt: thirty-two-bit right rotation expressed as a kernel C-DSL unit.
pub extern "c" def Stigi.Raw.rotateRightWord(word: U32, shift: U32) <below(shift, 32)>: U32 =
  U32.rotateRight(word, shift)
