// prompt: implicit equality instance for unsigned bytes.
pub given Stigi.Equal.Byte: Stigi.Equal[U8]:
  def equal(left: U8, right: U8): Bool = left == right
