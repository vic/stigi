// prompt: whether two unit references name the same unit member.
def Stigi.Types.sameReference(left: Stigi.Core.Reference, right: Stigi.Core.Reference): Bool =
  left match
    case ExternalRef(leftUnit, leftMember) =>
      right match
        case ExternalRef(rightUnit, rightMember) => leftUnit == rightUnit && leftMember == rightMember
        case SelfGroupRef(_) | InnerRef(_) => false
    case SelfGroupRef(leftMember) =>
      right match
        case SelfGroupRef(rightMember) => leftMember == rightMember
        case ExternalRef(_, _) | InnerRef(_) => false
    case InnerRef(leftMember) =>
      right match
        case InnerRef(rightMember) => leftMember == rightMember
        case ExternalRef(_, _) | SelfGroupRef(_) => false
