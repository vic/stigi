// prompt: one local of a unit body: a binder with its name text and the capability it stands for, or a unit alias with its name text and the target run it rewrites to.
enum Stigi.Names.Local:
  case Local(text: Stigi.Syntax.Text, binding: Stigi.Names.CapabilityBinding)
  case AliasLocal(text: Stigi.Syntax.Text, targetFirst: U64, targetEnd: U64) <lessOrEqual(targetFirst, targetEnd), lessOrEqual(targetEnd, Stigi.Limit.maxUnitBytes)>
