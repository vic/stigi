// prompt: why the walk of a unit stopped early: it did not, or its task stack, its scope or its node table failed, or it ran out of steps.
enum Stigi.Names.WalkFault:
  case WalkSound
  case StackFull
  case ScopeFull
  case TableMalformed
  case StepsExhausted
