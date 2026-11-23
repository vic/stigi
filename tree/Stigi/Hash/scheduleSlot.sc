// prompt: official BLAKE3 message schedule permutation index.
def Stigi.Hash.scheduleSlot(slot: U64) <below(slot, 16)>: U64 <below(result, 16)> =
  case 0 => 2
  case 1 => 6
  case 2 => 3
  case 3 => 10
  case 4 => 7
  case 5 => 0
  case 6 => 4
  case 7 => 13
  case 8 => 1
  case 9 => 11
  case 10 => 12
  case 11 => 5
  case 12 => 9
  case 13 => 14
  case 14 => 15
  case _ => 8
