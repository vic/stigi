// prompt: the state of one probe of the type hash index: still probing, an equal type found, or an empty slot found.
enum Stigi.Types.Probe:
  case Probing
  case FoundEqual(typeAt: Stigi.Core.TypeAt)
  case FoundVacant
