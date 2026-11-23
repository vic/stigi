// prompt: implicit equality instance for boolean flags.
pub given Stigi.Equal.Flag: Stigi.Equal[Bool]:
  def equal(left: Bool, right: Bool): Bool = left == right
