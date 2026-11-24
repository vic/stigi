// prompt: what a lookup of a local binder by its text finds: nothing, or the capability the binder carries.
enum Stigi.Names.LocalLookup:
  case NotLocal
  case LocalBound(binding: Stigi.Names.CapabilityBinding)
