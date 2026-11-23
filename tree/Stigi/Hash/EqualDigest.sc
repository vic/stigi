// prompt: implicit structural equality instance for BLAKE3 digests.
pub given Stigi.Hash.EqualDigest: Stigi.Equal[Stigi.Hash.Digest]:
  def equal(left: Stigi.Hash.Digest, right: Stigi.Hash.Digest): Bool =
    case (Digest(leftZero, leftOne, leftTwo, leftThree, leftFour, leftFive, leftSix, leftSeven), Digest(rightZero, rightOne, rightTwo, rightThree, rightFour, rightFive, rightSix, rightSeven)) =>
      leftZero == rightZero && leftOne == rightOne && leftTwo == rightTwo && leftThree == rightThree && leftFour == rightFour && leftFive == rightFive && leftSix == rightSix && leftSeven == rightSeven
