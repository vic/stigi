// prompt: return one of the eight standard BLAKE3 initialization vector words.
def Stigi.Hash.initWord(index: U64) <below(index, 8)>: U32 =
  if index == 0 then
    0x6A09E667
  else
    if index == 1 then
      0xBB67AE85
    else
      if index == 2 then
        0x3C6EF372
      else
        if index == 3 then
          0xA54FF53A
        else
          if index == 4 then
            0x510E527F
          else
            if index == 5 then
              0x9B05688C
            else
              if index == 6 then
                0x1F83D9AB
              else
                0x5BE0CD19
