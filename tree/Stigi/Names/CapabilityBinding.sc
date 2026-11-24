// prompt: what capability a local binder stands for: none, a builtin capability, or a capability unit of the image.
enum Stigi.Names.CapabilityBinding:
  case WithoutCapability
  case BuiltinCapability
  case CapabilityUnit(entry: U64)
