// prompt: check exact equality of two 256-bit BLAKE3 digests.
pub def Stigi.Hash.digestsEqual(left: Stigi.Hash.Digest, right: Stigi.Hash.Digest) <lessOrEqual(0, 1)>: Bool =
  val DigestWords(leftZero, leftOne, leftTwo, leftThree, leftFour, leftFive, leftSix, leftSeven) = left
  val DigestWords(rightZero, rightOne, rightTwo, rightThree, rightFour, rightFive, rightSix, rightSeven) = right
  val lowMatch: Bool = leftZero == rightZero && leftOne == rightOne && leftTwo == rightTwo && leftThree == rightThree
  val highMatch: Bool = leftFour == rightFour && leftFive == rightFive && leftSix == rightSix && leftSeven == rightSeven
  lowMatch && highMatch
