// prompt: compare two 256-bit BLAKE3 digests lexicographically by little-endian word order.
pub def Stigi.Hash.digestCompare(left: Stigi.Hash.Digest, right: Stigi.Hash.Digest) <lessOrEqual(0, 1)>: I32 =
  val DigestWords(leftZero, leftOne, leftTwo, leftThree, leftFour, leftFive, leftSix, leftSeven) = left
  val DigestWords(rightZero, rightOne, rightTwo, rightThree, rightFour, rightFive, rightSix, rightSeven) = right
  if leftZero != rightZero then
    if leftZero < rightZero then -1 else 1
  else
    if leftOne != rightOne then
      if leftOne < rightOne then -1 else 1
    else
      if leftTwo != rightTwo then
        if leftTwo < rightTwo then -1 else 1
      else
        if leftThree != rightThree then
          if leftThree < rightThree then -1 else 1
        else
          if leftFour != rightFour then
            if leftFour < rightFour then -1 else 1
          else
            if leftFive != rightFive then
              if leftFive < rightFive then -1 else 1
            else
              if leftSix != rightSix then
                if leftSix < rightSix then -1 else 1
              else
                if leftSeven != rightSeven then
                  if leftSeven < rightSeven then -1 else 1
                else
                  0
