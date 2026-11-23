// prompt: implicit equality instance for unsigned sixty-four-bit words.
pub given Stigi.Equal.Word: Stigi.Equal[U64]:
  def equal(left: U64, right: U64): Bool = left == right
