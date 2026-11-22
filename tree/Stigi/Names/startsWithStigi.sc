// prompt: check whether a unit name begins with the reserved Stigi kernel namespace prefix.
def Stigi.Names.startsWithStigi(length: U64, name: Array[U8](length)) <below(length, 0x100000000)>: Bool =
  if length >= 6 then
    name(0) == 83 && name(1) == 116 && name(2) == 105 && name(3) == 103 && name(4) == 105 && name(5) == 46
  else
    false
