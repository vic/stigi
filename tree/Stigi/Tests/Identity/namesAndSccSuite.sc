// prompt: test namespace visibility, case-pair detection, root reachability, and dependency-first SCC ordering.
val Stigi.Tests.Identity.namesAndSccSuite {Stigi.Test}: Stigi.Test.Result =
  val callerChild: Array[U8](16) = "App.Net.Sub.send"
  val targetPriv: Array[U8](12) = "App.Net.dial"
  val callerOther: Array[U8](12) = "App.Log.info"
  val sameVis: Bool = Stigi.Names.sameOrAncestorNamespace(16, callerChild, 12, targetPriv)
  val otherVis: Bool = Stigi.Names.sameOrAncestorNamespace(12, callerOther, 12, targetPriv)
  val visPriv: Stigi.Names.VisibilityOutcome = Stigi.Names.checkVisibility(12, callerOther, 12, targetPriv, false, false)
  val privRejected: Bool = visPriv match
    case PrivateReference => true
    case Visible => false
    case ReservedName => false
  val kernelTarget: Array[U8](14) = "Stigi.Core.run"
  val visReserved: Stigi.Names.VisibilityOutcome = Stigi.Names.checkVisibility(14, kernelTarget, 12, targetPriv, true, false)
  val reservedRejected: Bool = visReserved match
    case ReservedName => true
    case Visible => false
    case PrivateReference => false
  val nameUpper: Array[U8](8) = "Demo.Sum"
  val nameLower: Array[U8](8) = "demo.sum"
  val caseCollide: Bool = Stigi.Names.caseFoldEquals(8, nameUpper, 8, nameLower)
  val zeroWord: U64 = 0
  val zeroByte: U8 = 0
  val graphValid: Bool = Array.tryFill{Stigi.Region}(4, zeroWord) match
    case FillFailed => false
    case Filled(edgeFrom) =>
      Array.tryFill{Stigi.Region}(4, zeroWord) match
        case FillFailed => false
        case Filled(edgeTo) =>
          Array.tryFill{Stigi.Region}(4, zeroByte) match
            case FillFailed => false
            case Filled(isRoot) =>
              Array.tryFill{Stigi.Region}(4, zeroByte) match
                case FillFailed => false
                case Filled(reachable) =>
                  Array.tryFill{Stigi.Region}(4, zeroWord) match
                    case FillFailed => false
                    case Filled(compOf) =>
                      edgeFrom(0) := 0
                      edgeTo(0) := 1
                      edgeFrom(1) := 1
                      edgeTo(1) := 2
                      edgeFrom(2) := 2
                      edgeTo(2) := 1
                      edgeFrom(3) := 2
                      edgeTo(3) := 3
                      isRoot(0) := 1
                      val reached: U64 = Stigi.Names.computeReachable{Stigi.Region}(4, 4, edgeFrom, edgeTo, isRoot, reachable)
                      val comps: U64 = Stigi.Names.computeComponents{Stigi.Region}(4, 4, edgeFrom, edgeTo, compOf)
                      val cycleGrouped: Bool = compOf(1) == compOf(2)
                      val depOrdered: Bool = compOf(3) < compOf(1) && compOf(1) < compOf(0)
                      reached == 4 && comps == 3 && cycleGrouped && depOrdered
  val allPassed: Bool = sameVis && !otherVis && privRejected && reservedRejected && caseCollide && graphValid
  Stigi.Test.expect(allPassed, true, "Names phase checks visibility, case-pair, reachability, and dependency-first SCCs")
