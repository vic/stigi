// prompt: what a local binder stands for: no capability, a builtin capability, or a capability unit of the image.
enum Stigi.Names.CapabilityBinding:
  case WithoutCapability
  case BuiltinCapability
  case CapabilityUnit(entry: U64) <below(entry, Stigi.Limit.maxUnits)>
