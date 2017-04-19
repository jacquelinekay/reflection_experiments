#pragma once

#include <utility>
namespace jk {

template<typename Callable, typename X0, typename X1, typename ...Xs>
struct fold_right_helper {
  static constexpr decltype(auto) fold(Callable&& f, Xs&&... xs, X0&& x0, X1&& x1) {
    return ([&f, acc = f(x0, x1)](auto y) mutable
      {
        return acc = f(y, acc);
      }(xs), ...);
  }
};

template<typename Callable, typename X0, typename X1, typename ...Xs>
constexpr auto fold_right(Callable&& f, X0&& x0, X1&& x1, Xs&&... xs) {
  return fold_right_helper<Callable, X0, X1, Xs...>::fold(f, x0, x1, xs...);
}

template<typename Callable, typename X0, typename X1, typename ...Xs>
constexpr decltype(auto) fold_left(Callable&& f, X0&& x0, X1&& x1, Xs&&... xs) {
  return (..., [&f, acc = f(x0, x1)](auto y) mutable
    {
      return acc = f(acc, y);
    }(xs));
}

template<typename Callable, typename X0, typename X1, typename ...Xs>
constexpr decltype(auto) fold(Callable&& f, X0&& x0, X1&& x1, Xs&&... xs) {
  return fold_left(f, x0, x1, xs...);
}

}  // namespace jk
