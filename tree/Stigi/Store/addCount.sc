// prompt: add two counts when both stay below the source byte limit, otherwise keep the first count.
def Stigi.Store.addCount(count: U64, added: U64) <below(0, Stigi.Limit.maxSourceBytes)>: U64 =
  if count < Stigi.Limit.maxSourceBytes && added < Stigi.Limit.maxSourceBytes then count + added else count
