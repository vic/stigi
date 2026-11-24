// prompt: represent a resolved unit reference in the Checked IR.
pub enum Stigi.Core.Reference:
  case ExternalRef(unitSlot: U64, memberSlot: U64)
  case SelfGroupRef(memberSlot: U64)
  case InnerRef(memberSlot: U64)
