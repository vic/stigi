// prompt: the byte span of a dotted unit name, or no name.
enum Stigi.Parse.NameSpan:
  case Named(nameStart: U64, nameEnd: U64) <lessOrEqual(nameStart, nameEnd)>
  case Unnamed
