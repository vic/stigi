// prompt: verify redundant branch already decided by facts rejects with BranchDecided and NeverReached.
val Stigi.NegTests.branchDecided {Stigi.NegTest}: Never =
  def Stigi.NegTests.badBranch(item: U64, limit: U64) <below(item, limit)>: U64 =
    if item < limit then item else limit
  Stigi.NegTest.expect(Stigi.Code.BranchDecided, "item < limit is already proved by precondition")
  Stigi.NegTest.expect(Stigi.Code.NeverReached, "false branch is dead when condition is decided")
