// prompt: evaluate namespace visibility and kernel namespace reservation for a reference edge.
pub def Stigi.Names.checkVisibility(callerLen: U64, callerName: Array[U8](callerLen), targetLen: U64, targetName: Array[U8](targetLen), targetPublic: Bool, kernelMode: Bool) <below(callerLen, 0x100000000), below(targetLen, 0x100000000)>: Stigi.Names.VisibilityOutcome =
  val callerKernel: Bool = Stigi.Names.startsWithStigi(callerLen, callerName)
  if !kernelMode && callerKernel then
    Stigi.Names.VisibilityOutcome.ReservedName
  else
    if targetPublic then
      Stigi.Names.VisibilityOutcome.Visible
    else
      if Stigi.Names.sameOrAncestorNamespace(callerLen, callerName, targetLen, targetName) then
        Stigi.Names.VisibilityOutcome.Visible
      else
        Stigi.Names.VisibilityOutcome.PrivateReference
