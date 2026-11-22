// prompt: classify stored unit declaration kinds recorded in the tree index.
pub enum Stigi.Store.UnitKind:
  case KindEnum
  case KindType
  case KindDef
  case KindLemma
  case KindVal
  case KindCapability
  case KindHandler
  case KindExtern
  case KindRelation
  case KindRule
  case KindTest
  case KindNegTest
  case KindUnknown
