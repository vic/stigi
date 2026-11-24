// prompt: compress one BLAKE3 parent node of a stacked left chain and the current right chain, at most fifty-six stacked subtrees deep.
def Stigi.Hash.compressParent {Stigi.Mut} (work: Stigi.Hash.Work, parentDepth: U64, isRoot: Bool) <below(parentDepth, 56)>: Unit =
  val Work(chain, block, _, _, _, stack, _) = work
  val leftStart: U64 = parentDepth * 8
  val leftEnd: U64 = (parentDepth + 1) * 8
  if leftStart <= leftEnd && leftEnd <= 512 then
    Stigi.Array.copy(512, stack, leftStart, leftEnd, 16, block, 0)
    ()
  else ()
  Stigi.Array.copy(8, chain, 0, 8, 16, block, 8)
  Stigi.Hash.resetChain(chain)
  Stigi.Hash.compressBlock(work, 0, U32.ofU64(64), if isRoot then U32.bitwiseOr(Stigi.Hash.parentFlag, Stigi.Hash.rootFlag) else Stigi.Hash.parentFlag)
