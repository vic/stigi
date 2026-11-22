// prompt: check whether callerName resides in the same or a descendant namespace of targetName.
pub def Stigi.Names.sameOrAncestorNamespace(callerLen: U64, callerName: Array[U8](callerLen), targetLen: U64, targetName: Array[U8](targetLen)) <below(callerLen, 0x100000000), below(targetLen, 0x100000000)>: Bool =
  val targetDot: U64 = Stigi.Names.lastDotOffset(targetLen, targetName)
  if targetDot == targetLen then
    true
  else
    val callerDot: U64 = Stigi.Names.lastDotOffset(callerLen, callerName)
    if callerDot == callerLen then
      false
    else
      if callerDot == targetDot then
        Stigi.Names.prefixBytesEqual(callerLen, callerName, targetLen, targetName, targetDot)
      else
        if targetDot < callerDot then
          val samePrefix: Bool = Stigi.Names.prefixBytesEqual(callerLen, callerName, targetLen, targetName, targetDot)
          samePrefix && callerName(targetDot) == 46
        else
          false
