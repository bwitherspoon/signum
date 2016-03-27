/*
 * Copyright 2015 C. Brett Witherspoon
 */

#ifndef SIGNUM_MATH_HPP_
#define SIGNUM_MATH_HPP_

#include <type_traits>

namespace signum
{
namespace math
{

/**
 * \brief Return the absolute value of the argument.
 *
 * Until C++14 the STL version of this function is not constexpr, so we use
 * this.
 *
 * \param v the argument
 * \return the absolute value of v
 *
 */
template<typename T>
constexpr T abs(T v)
{
  static_assert(std::is_arithmetic<T>::value,
      "template argument must be an arithmetic type.");

  return (v < 0) ? -v : v;
}

/**
 * \brief Return the maximum of the arguments
 *
 * Until C++14 the STL version of this function is not constexpr, so we use
 * this.
 *
 * \param a an argument
 * \param b an argument
 * \return the maximum of a and b
 */
template<typename T>
constexpr T max(T a, T b)
{
  static_assert(std::is_arithmetic<T>::value,
      "template argument must be an arithmetic type.");

  return (a > b) ? a : b;
}

/**
 * \brief Compute the greatest common divisor of the arguments
 * \param a an integer
 * \param b an integer
 * \return the greatest common divisor of a and b
 */
template<typename T>
constexpr T gcd(T a, T b)
{
  static_assert(std::is_integral<T>::value,
      "template argument must be an integral type.");

  return (b == 0) ? a : gcd(b, a % b);
}

/**
 * \brief Compute the least common multiple of the aruments
 * \param a an integer
 * \param b an integer
 * \return the least common multiple of a and b
 */
template <typename T>
constexpr T lcm(T a, T b)
{
  return (a == 0 and b == 0) ? 0 : (abs(a) / gcd(a, b)) * abs(b);
}

} /* namespace math */
} /* namespace signum */
#endif /* SIGNUM_MATH_HPP_ */