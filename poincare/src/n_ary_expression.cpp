#include <poincare/n_ary_expression.h>
#include <poincare/number.h>
extern "C" {
#include <assert.h>
#include <stdlib.h>
}

namespace Poincare {

bool NAryExpressionNode::childNeedsUserParentheses(const Expression & child) const {
  /* Expressions like "-2" require parentheses in Addition/Multiplication except
   * when they are the first operand. */
  if (((child.isNumber() && static_cast<const Number &>(child).sign() == Sign::Negative)
        || child.type() == Type::Opposite)
        /* We use "hasAncestor" instead of "==" because child might not be the
         * direct child of the addition/multiplication [e.g. +(conj(-2), 3)] */
       && !child.node()->hasAncestor(childAtIndex(0), true)) {
    return true;
  }
  if (child.type() == Type::Conjugate) {
    return childNeedsUserParentheses(child.childAtIndex(0));
  }
  return false;
}

void NAryExpressionNode::sortChildrenInPlace(ExpressionOrder order, Context * context, bool canSwapMatrices, bool canBeInterrupted) {
  Expression reference(this);
  for (int i = reference.numberOfChildren()-1; i > 0; i--) {
    bool isSorted = true;
    for (int j = 0; j < reference.numberOfChildren()-1; j++) {
      /* Warning: Matrix operations are not always commutative (ie,
       * multiplication) so we never swap 2 matrices. */
      ExpressionNode * cj = childAtIndex(j);
      ExpressionNode * cj1 = childAtIndex(j+1);
      if (order(cj, cj1, canBeInterrupted) > 0 && (canSwapMatrices || !(Expression(cj).deepIsMatrix(context) && Expression(cj1).deepIsMatrix(context)))) {
        reference.swapChildrenInPlace(j, j+1);
        isSorted = false;
      }
    }
    if (isSorted) {
      return;
    }
  }
}

Expression NAryExpressionNode::squashUnaryHierarchyInPlace() {
  NAryExpression reference = NAryExpression(this);
  if (reference.numberOfChildren() == 1) {
    Expression child = reference.childAtIndex(0);
    reference.replaceWithInPlace(child);
    return child;
  }
  return reference;
}

// Private

int NAryExpressionNode::simplificationOrderSameType(const ExpressionNode * e, bool ascending, bool canBeInterrupted) const {
  int m = numberOfChildren();
  int n = e->numberOfChildren();
  for (int i = 1; i <= m; i++) {
    // The NULL node is the least node type.
    if (n < i) {
      return 1;
    }
    int order = SimplificationOrder(childAtIndex(m-i), e->childAtIndex(n-i), ascending, canBeInterrupted);
    if (order != 0) {
      return order;
    }
  }
  // The NULL node is the least node type.
  if (n > m) {
    return ascending ? -1 : 1;
  }
  return 0;
}

int NAryExpressionNode::simplificationOrderGreaterType(const ExpressionNode * e, bool ascending, bool canBeInterrupted) const {
  int m = numberOfChildren();
  if (m == 0) {
    return -1;
  }
  /* Compare e to last term of hierarchy. */
  int order = SimplificationOrder(childAtIndex(m-1), e, ascending, canBeInterrupted);
  if (order != 0) {
    return order;
  }
  if (m > 1) {
    return ascending ? 1 : -1;
  }
  return 0;
}

int NAryExpression::allChildrenAreReal(Context * context) const {
  int i = 0;
  int result = 1;
  while (i < numberOfChildren()) {
    Expression c = childAtIndex(i);
    if (c.type() == ExpressionNode::Type::ComplexCartesian) {
      result *= 0;
    } else if (!c.isReal(context)) {
      return -1;
    }
    i++;
  }
  return result;
}

}
