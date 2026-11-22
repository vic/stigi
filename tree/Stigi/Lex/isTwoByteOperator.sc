// prompt: check whether two adjacent bytes form a two-character L0 operator token.
def Stigi.Lex.isTwoByteOperator(head: U8, follow: U8) <lessOrEqual(0, 1)>: Bool =
  val equalPair: Bool = head == 61 && (follow == 62 || follow == 61)
  val cmpPair: Bool = (head == 58 || head == 33 || head == 60 || head == 62) && follow == 61
  val logicPair: Bool = head == 38 && follow == 38 || head == 124 && follow == 124
  equalPair || cmpPair || logicPair
