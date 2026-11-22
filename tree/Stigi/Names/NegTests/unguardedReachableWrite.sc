// prompt: writing to the reachability table without proving node index bound is rejected.
val Stigi.Names.NegTests.unguardedReachableWrite {Stigi.NegTest}: Never =
  def markNode {Stigi.Mut} (nodeCount: U64, reachable: Array[U8](nodeCount), target: U64) <lessOrEqual(0, nodeCount)>: Unit =
    reachable(target) := 1
  Stigi.NegTest.expect(Stigi.Code.IndexUnproved, "target node index is not bounded below nodeCount")
