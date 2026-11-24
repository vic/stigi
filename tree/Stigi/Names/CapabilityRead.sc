// prompt: the result of reading the capability a binder stands for: the binding found, or a node table or image that cannot hold the declared type.
enum Stigi.Names.CapabilityRead:
  case CapabilityFound(binding: Stigi.Names.CapabilityBinding)
  case CapabilityMalformed
