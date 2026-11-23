// prompt: check whether two adjacent bytes form a two-character L0 operator token.
def Stigi.Emit.isTwoBytePunct(head: U8, follow: U8) <lessOrEqual(0, 1)>: Bool =
  head == 61 && follow == 61 || head == 33 && follow == 61 || head == 60 && follow == 61 || head == 62 && follow == 61 || head == 38 && follow == 38 || head == 124 && follow == 124 || head == 58 && follow == 61 || head == 61 && follow == 62
