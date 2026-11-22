// prompt: compute the official 256-bit BLAKE3 digest of an arbitrary byte slice.
pub def Stigi.Hash.hashBytes {region: Stigi.Region, Stigi.Mut} (capacity: U64, input: Array[U8](capacity), length: U64) <lessOrEqual(length, capacity), below(capacity, 0x100000000)>: Stigi.Hash.Digest =
  val zeroWord: U32 = 0
  Array.tryFill{region}(8, zeroWord) match
    case FillFailed => Stigi.Hash.Digest.DigestWords(0, 0, 0, 0, 0, 0, 0, 0)
    case Filled(chainWords) =>
      Array.tryFill{region}(16, zeroWord) match
        case FillFailed => Stigi.Hash.Digest.DigestWords(0, 0, 0, 0, 0, 0, 0, 0)
        case Filled(message) =>
          Array.tryFill{region}(16, zeroWord) match
            case FillFailed => Stigi.Hash.Digest.DigestWords(0, 0, 0, 0, 0, 0, 0, 0)
            case Filled(scratch) =>
              Array.tryFill{region}(16, zeroWord) match
                case FillFailed => Stigi.Hash.Digest.DigestWords(0, 0, 0, 0, 0, 0, 0, 0)
                case Filled(state) =>
                  if length <= 1024 then
                    Stigi.Hash.compressChunk(capacity, input, 0, length, 0, true, chainWords, message, scratch, state)
                    Stigi.Hash.Digest.DigestWords(chainWords(0), chainWords(1), chainWords(2), chainWords(3), chainWords(4), chainWords(5), chainWords(6), chainWords(7))
                  else
                    Array.tryFill{region}(512, zeroWord) match
                      case FillFailed => Stigi.Hash.Digest.DigestWords(0, 0, 0, 0, 0, 0, 0, 0)
                      case Filled(stackWords) =>
                        Array.tryFill{region}(8, zeroWord) match
                          case FillFailed => Stigi.Hash.Digest.DigestWords(0, 0, 0, 0, 0, 0, 0, 0)
                          case Filled(leftChain) =>
                            Array.tryFill{region}(8, zeroWord) match
                              case FillFailed => Stigi.Hash.Digest.DigestWords(0, 0, 0, 0, 0, 0, 0, 0)
                              case Filled(initChain) =>
                                Stigi.Hash.hashMultiChunk(capacity, input, length, chainWords, message, scratch, state, stackWords, leftChain, initChain)
