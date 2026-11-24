// prompt: compute official BLAKE3 digest over a bounded byte slice with chunk tree chaining.
pub def Stigi.Hash.hashBytes {region: Stigi.Region, Stigi.Mut} (cap: Stigi.ByteCap, data: Array[U8](cap), length: U64) <lessOrEqual(length, cap)>: Stigi.Hash.HashOutcome =
  val Filled(chain) = Array.tryFill[U32]{region}(8, U32.ofU64(0)) else
    case FillFailed => HashOutOfMemory
  val Filled(block) = Array.tryFill[U32]{region}(16, U32.ofU64(0)) else
    case FillFailed => HashOutOfMemory
  val Filled(state) = Array.tryFill[U32]{region}(16, U32.ofU64(0)) else
    case FillFailed => HashOutOfMemory
  val Filled(msg) = Array.tryFill[U32]{region}(16, U32.ofU64(0)) else
    case FillFailed => HashOutOfMemory
  val Filled(scratch) = Array.tryFill[U32]{region}(16, U32.ofU64(0)) else
    case FillFailed => HashOutOfMemory
  val Filled(stack) = Array.tryFill[U32]{region}(512, U32.ofU64(0)) else
    case FillFailed => HashOutOfMemory
  val Filled(padded) = Array.tryFill[U8]{region}(64, 0) else
    case FillFailed => HashOutOfMemory
  Stigi.Hash.hashChunks(cap, data, length, Work(chain, block, state, msg, scratch, stack, padded), Tree(0, 0, 0), length + 1) match
    case TreeBuilt => Hashed(Digest(chain(0), chain(1), chain(2), chain(3), chain(4), chain(5), chain(6), chain(7)))
    case TreeTooDeep => HashOutOfMemory
