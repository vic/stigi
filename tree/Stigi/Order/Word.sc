// prompt: implicit ordering instance for unsigned sixty-four-bit words.
pub given Stigi.Order.Word: Stigi.Order[U64]:
  def lessThan(left: U64, right: U64): Bool = left < right
