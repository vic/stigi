// prompt: compare two 256-bit unit digests stored in four columnar U64 arrays.
def Stigi.Emit.digestSlotLess(unitCount: U64, digestLow: Array[U64](unitCount), digestMid: Array[U64](unitCount), digestHigh: Array[U64](unitCount), digestTop: Array[U64](unitCount), leftSlot: U64, rightSlot: U64) <below(leftSlot, unitCount), below(rightSlot, unitCount)>: Bool =
  if digestLow(leftSlot) != digestLow(rightSlot) then
    digestLow(leftSlot) < digestLow(rightSlot)
  else if digestMid(leftSlot) != digestMid(rightSlot) then
    digestMid(leftSlot) < digestMid(rightSlot)
  else if digestHigh(leftSlot) != digestHigh(rightSlot) then
    digestHigh(leftSlot) < digestHigh(rightSlot)
  else
    digestTop(leftSlot) < digestTop(rightSlot)
