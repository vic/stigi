// prompt: strict total ordering trait resolved by kernel uKanren search.
trait Stigi.Order [A: Type]:
  def lessThan(left: A, right: A): Bool
