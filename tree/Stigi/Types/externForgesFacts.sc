// prompt: enforce the extern-forges-facts rule on C ABI returns and overwritten buffer elements.
pub def Stigi.Types.externForgesFacts(returnHasFacts: Bool, overwritesBuffer: Bool, bufferElemHasFacts: Bool, bufferElemIsEnum: Bool) <lessOrEqual(0, 1)>: Bool =
  returnHasFacts || overwritesBuffer && (bufferElemHasFacts || bufferElemIsEnum)
