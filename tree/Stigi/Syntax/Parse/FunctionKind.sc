// prompt: the kind of a declaration with a def head: a def, a lemma def, or an extern def with its interface.
enum Stigi.Syntax.Parse.FunctionKind:
  case PlainFunction
  case LemmaFunction
  case ExternFunction(abi: Stigi.Syntax.Text)
