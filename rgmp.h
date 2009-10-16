/*
    rGMP is yet another GMP wrapper for Ruby
    Copyright (C) 2009  Ralf Gunter

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "gmp.h"
#include "ruby.h"

#ifdef MPFR
#include "mpfr.h"
#include "mpf2mpfr.h"
#endif

extern VALUE mGMP;
extern VALUE cGMPInteger, cGMPFloat;


/* GMP::Integer method prototyping */

// Initialization function
extern void Init_gmpz();

// Garbage collection
extern VALUE integer_mark(mpz_t*);
extern VALUE integer_free(mpz_t*);

// Object allocation
extern VALUE integer_allocate(VALUE);

// Class constructor
extern VALUE z_init(VALUE, VALUE);

// Conversion methods
extern VALUE z_to_string(VALUE, VALUE*, VALUE);
extern VALUE z_to_integer(VALUE);
extern VALUE z_to_float(VALUE);

// Binary arithmetical operators
extern VALUE z_addition(VALUE, VALUE);
extern VALUE z_subtraction(VALUE, VALUE);
extern VALUE z_multiplication(VALUE, VALUE);
extern VALUE z_division(VALUE, VALUE);
extern VALUE z_modulo(VALUE, VALUE);
extern VALUE z_power(VALUE, VALUE);
extern VALUE z_left_shift(VALUE, VALUE);
extern VALUE z_right_shift(VALUE, VALUE);

//Unary arithmetical operators
extern VALUE z_positive(VALUE);
extern VALUE z_negation(VALUE);

// Logic operators
extern VALUE z_logic_and(VALUE, VALUE);
extern VALUE z_logic_ior(VALUE, VALUE);
extern VALUE z_logic_xor(VALUE, VALUE);
extern VALUE z_logic_not(VALUE);

// Comparison methods
extern VALUE z_equality_test(VALUE, VALUE);
extern VALUE z_greater_than_test(VALUE, VALUE);
extern VALUE z_less_than_test(VALUE, VALUE);
extern VALUE z_greater_than_or_equal_to_test(VALUE, VALUE);
extern VALUE z_less_than_or_equal_to_test(VALUE, VALUE);
extern VALUE z_generic_comparison(VALUE, VALUE);

// In-place methods
extern VALUE z_next_prime_inplace(VALUE);
extern VALUE z_absolute_inplace(VALUE);
extern VALUE z_negation_inplace(VALUE);
extern VALUE z_sqrt_inplace(VALUE);
extern VALUE z_root_inplace(VALUE, VALUE);
extern VALUE z_invert_inplace(VALUE, VALUE);
extern VALUE z_set_bit_inplace(VALUE, VALUE, VALUE);
extern VALUE z_addition_inplace(VALUE, VALUE);
extern VALUE z_subtraction_inplace(VALUE, VALUE);
extern VALUE z_multiplication_inplace(VALUE, VALUE);

// Question-like methods
extern VALUE z_divisible(VALUE, VALUE);
extern VALUE z_perfect_power(VALUE);
extern VALUE z_perfect_square(VALUE);
extern VALUE z_probable_prime(VALUE, VALUE);
extern VALUE z_even(VALUE);
extern VALUE z_odd(VALUE);
extern VALUE z_precise_equality(VALUE, VALUE);
extern VALUE z_zero(VALUE);
extern VALUE z_nonzero(VALUE);

// Other operations
extern VALUE z_absolute(VALUE);
extern VALUE z_next_prime(VALUE);
extern VALUE z_size_in_base(VALUE, VALUE);
extern VALUE z_swap(VALUE, VALUE);
extern VALUE z_next(VALUE);
extern VALUE z_get_bit(VALUE, VALUE);

// Singletons/Class methods
extern VALUE z_powermod(VALUE, VALUE, VALUE, VALUE);
extern VALUE z_sqrt_singleton(VALUE, VALUE);
extern VALUE z_root_singleton(VALUE, VALUE, VALUE);
extern VALUE z_fibonacci_singleton(VALUE, VALUE);
extern VALUE z_lucas_singleton(VALUE, VALUE);
extern VALUE z_factorial_singleton(VALUE, VALUE);
extern VALUE z_binomial_singleton(VALUE, VALUE, VALUE);
extern VALUE z_remove_singleton(VALUE, VALUE, VALUE);
extern VALUE z_comp_abs_singleton(VALUE, VALUE, VALUE);
extern VALUE z_invert_singleton(VALUE, VALUE, VALUE);
extern VALUE z_lcm_singleton(VALUE, VALUE, VALUE);
extern VALUE z_gcd_singleton(VALUE, VALUE, VALUE);
extern VALUE z_jacobi_singleton(VALUE, VALUE, VALUE);



/* GMP::Float method prototyping */

// Initialization function
extern void Init_gmpf();

// Garbage collection
extern VALUE float_mark(mpz_t*);
extern VALUE float_free(mpz_t*);

// Object allocation
extern VALUE float_allocate(VALUE);

// Class constructor
extern VALUE f_init(VALUE, VALUE*, VALUE);

// Conversion methods
extern VALUE f_to_string(VALUE);
extern VALUE f_to_float(VALUE);

// Binary arithmetical operators
extern VALUE f_addition(VALUE, VALUE);
extern VALUE f_subtraction(VALUE, VALUE);
extern VALUE f_multiplication(VALUE, VALUE);
extern VALUE f_division(VALUE, VALUE);
extern VALUE f_power(VALUE, VALUE);

// Unary arithmetical operators
extern VALUE f_positive(VALUE);
extern VALUE f_negation(VALUE);

// Comparison methods
extern VALUE f_equality_test(VALUE, VALUE);
extern VALUE f_greater_than_test(VALUE, VALUE);
extern VALUE f_less_than_test(VALUE, VALUE);
extern VALUE f_greater_than_or_equal_to_test(VALUE, VALUE);
extern VALUE f_less_than_or_equal_to_test(VALUE, VALUE);
extern VALUE f_generic_comparison(VALUE, VALUE);

// Question-like methods
extern VALUE f_integer(VALUE);

// Other methods
extern VALUE f_set_precision(VALUE, VALUE);
extern VALUE f_get_precision(VALUE, VALUE);
extern VALUE f_swap(VALUE, VALUE);
extern VALUE f_absolute(VALUE, VALUE);
extern VALUE f_relative_difference(VALUE, VALUE);
extern VALUE f_ceil(VALUE);
extern VALUE f_floor(VALUE);
extern VALUE f_truncate(VALUE);

// Singletons/Class methods
extern VALUE f_set_def_prec(VALUE, VALUE);
extern VALUE f_get_def_prec(VALUE);
extern VALUE f_sqrt_singleton(VALUE);

#ifdef MPFR
// Trigonometric functions
extern VALUE f_sine(VALUE, VALUE);
extern VALUE f_cossine(VALUE, VALUE);
extern VALUE f_tangent(VALUE, VALUE);
extern VALUE f_cotangent(VALUE, VALUE);
extern VALUE f_secant(VALUE, VALUE);
extern VALUE f_cosecant(VALUE, VALUE);

// Logarithm methods
extern VALUE f_logn(VALUE, VALUE);
extern VALUE f_log2(VALUE, VALUE);
extern VALUE f_log10(VALUE, VALUE);

// Exponentiation methods
extern VALUE f_exp(VALUE, VALUE);
extern VALUE f_exp2(VALUE, VALUE);
extern VALUE f_exp10(VALUE, VALUE);
#endif
