// prompt: say how one pattern name resolves: to the ordinal of an enum case or to a local binder.
enum Stigi.Elaborate.PatResolution:
  case PatCaseOrd(caseIndex: U64)
  case PatLocalBind
