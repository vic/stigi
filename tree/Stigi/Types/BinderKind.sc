// prompt: classify a length-term binder as an immutable parameter or val, a forbidden mutable var, or a known linear form.
pub enum Stigi.Types.BinderKind:
  case ImmutableBinder
  case MutableBinder
  case KnownBinder(form: Stigi.Types.Linear.Form)
