// prompt: compare two byte runs of the name pool.
def Stigi.Names.poolSame(pool: Array[U8](Stigi.Limit.maxIndexBytes), left: U64, right: U64, count: U64) <decreases(count)>: Bool =
  if count < 1 then true else
    if Stigi.Limit.maxIndexBytes <= left || Stigi.Limit.maxIndexBytes <= right then false else
      if pool(left) == pool(right) then Stigi.Names.poolSame(pool, left + 1, right + 1, count - 1) else false
