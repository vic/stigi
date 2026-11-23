// prompt: structural equality trait resolved by kernel uKanren search.
trait Stigi.Equal [A: Type]:
  def equal(left: A, right: A): Bool
