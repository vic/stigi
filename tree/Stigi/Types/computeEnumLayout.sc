// prompt: compute the C11 layout of an enum choosing 1-word nullable pointer or tail-padding tag when legal.
pub def Stigi.Types.computeEnumLayout(caseCount: U64, maxPayloadSize: U64, maxPayloadAlign: U64, emptyCaseCount: U64, singleHandlePayload: Bool, payloadTailPadding: U64) <below(0, caseCount), below(caseCount, 256), below(maxPayloadSize, 0x100000000), below(0, maxPayloadAlign), below(maxPayloadAlign, 4096), below(payloadTailPadding, 4096)>: Stigi.Types.EnumLayout =
  if caseCount == 2 && emptyCaseCount == 1 && singleHandlePayload then
    Stigi.Types.EnumLayout.LayoutNullablePtr(0, 8, 8)
  else
    val tagBytes: U64 = 1
    if maxPayloadSize > 0 && payloadTailPadding >= tagBytes && payloadTailPadding <= maxPayloadSize then
      val tagOff: U64 = maxPayloadSize - payloadTailPadding
      val totalSize: U64 = Stigi.Types.alignForward(maxPayloadSize, maxPayloadAlign)
      Stigi.Types.EnumLayout.LayoutTailTag(totalSize, maxPayloadAlign, tagOff)
    else
      val payloadOff: U64 = Stigi.Types.alignForward(tagBytes, maxPayloadAlign)
      if payloadOff < 4096 then
        val rawTotal: U64 = payloadOff + maxPayloadSize
        val alignedTotal: U64 = if rawTotal < 0x100000000 then Stigi.Types.alignForward(rawTotal, maxPayloadAlign) else rawTotal
        Stigi.Types.EnumLayout.LayoutTagged(alignedTotal, maxPayloadAlign, payloadOff)
      else
        Stigi.Types.EnumLayout.LayoutTagged(maxPayloadSize, maxPayloadAlign, payloadOff)
