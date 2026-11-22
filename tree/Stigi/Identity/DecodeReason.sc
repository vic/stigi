// prompt: strict canonical decoder rejection reasons from the identity specification.
pub enum Stigi.Identity.DecodeReason:
  case ReasonUnknownTag
  case ReasonTruncated
  case ReasonTrailingBytes
  case ReasonNonMinimalNumber
  case ReasonNumberTooLarge
  case ReasonUnboundLocal
  case ReasonMemberOutOfRange
  case ReasonNonCanonicalGroupOrder
  case ReasonWrongSchema
