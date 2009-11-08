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
#include "rgmp.h"

////////////////////////////////////////////////////////////////////
//// Fundamental methods
// Garbage collection
void
rational_mark( mpq_t *q ) {}

void
rational_free( mpq_t *q ) {
	mpq_clear(*q);
}

// Object allocation
VALUE
rational_allocate( VALUE klass ) {
	mpq_t *q = malloc(sizeof(mpq_t));
	mpq_init(*q);
	return Data_Wrap_Struct(klass, rational_mark, rational_free, q);
}

// Class constructor
VALUE
q_init( VALUE self, VALUE ratData ) {
	// Creates a mpq_t pointer and loads self into it.
	mpq_t *q;
	Data_Get_Struct(self, mpq_t, q);
	
	switch (TYPE(ratData)) {
		case T_DATA: {
			VALUE class = rb_obj_class(ratData);
			if (class == cGMPRational) {
				mpq_t *dr;
				Data_Get_Struct(ratData, mpq_t, dr);
				mpq_set(*q, *dr);
			} else if (class == cGMPInteger) {
				mpz_t *dz;
				Data_Get_Struct(ratData, mpz_t, dz);
				mpq_set_z(*q, *dz);
			} else if (class == cGMPFloat) {
#ifdef MPFR

// Horrible hack, but apparently necessary nonetheless
#undef mpf_t
typedef __mpf_struct mpf_t[1];

				mpfr_t *dfr;
				Data_Get_Struct(ratData, mpfr_t, dfr);
				mpf_t df;
				__gmpf_init(df);
				mpfr_get_f(df, *dfr, GMP_RNDN);
				mpq_set_f(*q, df);
				__gmpf_clear(df);

#define mpf_t mpfr_t

#else
				mpf_t *df;
				Data_Get_Struct(ratData, mpf_t, df);
				mpq_set_f(*q, *df);
#endif
			} else {
				rb_raise(rb_eTypeError, "input data type not supported");
			}
			break;
		}
		case T_STRING: {
			mpq_set_str(*q, StringValuePtr(ratData), 10);
			
			if (mpz_cmp_ui(mpq_denref(*q), 0) == 0)
				rb_raise(rb_eRuntimeError, "denominator cannot be zero");
			
			mpq_canonicalize(*q);
			break;
		}
		default: {
			rb_raise(rb_eTypeError, "input data type not supported");
		}
	}
	
	return Qnil;
}
//// end of fundamental methods
////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////
//// Conversion methods
// To String
// {} -> {String}
VALUE
q_to_string( VALUE argc, VALUE *argv, VALUE self ) {
	// Loads self into a mpq_t
	mpq_t *s;
	Data_Get_Struct(self, mpq_t, s);
	
	// Creates a placeholder for the optional argument
	VALUE base;
	
	// The base argument is optional, and can vary from 2 to 36
	rb_scan_args(argc, argv, "01", &base);
	
	// Ensures the base, if present, is a Fixnum
	if (!FIXNUM_P(base) && !NIL_P(base))
		rb_raise(rb_eTypeError, "base must be a fixnum");
		
	// If the base hasn't been defined, this defaults it to 10
	int intBase;
	if (base == Qnil)
		intBase = 10;
	else
		intBase = FIX2INT(base);
	
	// Checks if the base is within range
	if (!(intBase >= 2 && intBase <= 36))
		rb_raise(rb_eRangeError, "base out of range");
	
	char *intStr = mpq_get_str(NULL, intBase, *s);
	VALUE rStr = rb_str_new2(intStr);
	free(intStr);
	
	return rStr;
}

// To Float
// {} -> {Float}
VALUE
q_to_float( VALUE self ) {
	// Loads self into a mpq_t
	mpq_t *s;
	Data_Get_Struct(self, mpq_t, s);
	
	return rb_float_new(mpq_get_d(*s));
}

// To GMP::Float
// {} -> {GMP::Float}
VALUE
q_to_gmpf( VALUE self ) {
	// Loads self into a mpq_t
	mpq_t *s;
	Data_Get_Struct(self, mpq_t, s);
	
	mpf_t *f = malloc(sizeof(*f));
	mpf_init(*f);
	mpf_set_q(*f, *s);
	
	return Data_Wrap_Struct(cGMPFloat, float_mark, float_free, f);
}
//// end of conversion methods
////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////
//// Binary arithmetical operators
// Addition (+)
// {GMP::Rational} -> {GMP::Rational}
VALUE
q_addition( VALUE self, VALUE summand ) {
	// Creates pointers to self's and the result's mpq_t structures
	mpq_t *r = malloc(sizeof(*r));
	mpq_t *q;
	
	// Loads self into *q
	Data_Get_Struct(self, mpq_t, q);
	
	// Inits the result
	mpq_init(*r);
	
	// Decides what to do based on the summand's type/class
	switch (TYPE(summand)) {
		case T_DATA: {
			if (rb_obj_class(summand) == cGMPRational) {
				mpq_t *sr;
				Data_Get_Struct(summand, mpq_t, sr);
				mpq_add(*r, *q, *sr);
			} else {
				rb_raise(rb_eTypeError, "summand's type is not supported");
			}
			break;
		}
		default: {
			rb_raise(rb_eTypeError, "summand's type is not supported");
		}
	}
	
	return Data_Wrap_Struct(cGMPRational, rational_mark, rational_free, r);
}

// Subtraction (-)
// {GMP::Rational} -> {GMP::Rational}
VALUE
q_subtraction( VALUE self, VALUE subtrahend ) {
	// Creates pointers to self's and the result's mpq_t structures
	mpq_t *r = malloc(sizeof(*r));
	mpq_t *q;
	
	// Loads self into *q
	Data_Get_Struct(self, mpq_t, q);
	
	// Inits the result
	mpq_init(*r);
	
	// Decides what to do based on the subtrahend's type/class
	switch (TYPE(subtrahend)) {
		case T_DATA: {
			if (rb_obj_class(subtrahend) == cGMPRational) {
				mpq_t *sr;
				Data_Get_Struct(subtrahend, mpq_t, sr);
				mpq_sub(*r, *q, *sr);
			} else {
				rb_raise(rb_eTypeError, "subtrahend's type is not supported");
			}
			break;
		}
		default: {
			rb_raise(rb_eTypeError, "subtrahend's type is not supported");
		}
	}
	
	return Data_Wrap_Struct(cGMPRational, rational_mark, rational_free, r);
}

// Multiplication (*)
// {GMP::Rational} -> {GMP::Rational}
VALUE
q_multiplication( VALUE self, VALUE multiplicand ) {
	// Creates pointers to self's and the result's mpq_t structures
	mpq_t *r = malloc(sizeof(*r));
	mpq_t *q;
	
	// Loads self into *q
	Data_Get_Struct(self, mpq_t, q);
	
	// Inits the result
	mpq_init(*r);
	
	// Decides what to do based on the multiplicand's type/class
	switch (TYPE(multiplicand)) {
		case T_DATA: {
			if (rb_obj_class(multiplicand) == cGMPRational) {
				mpq_t *sr;
				Data_Get_Struct(multiplicand, mpq_t, sr);
				mpq_mul(*r, *q, *sr);
			} else {
				rb_raise(rb_eTypeError, "multiplicand's type is not supported");
			}
			break;
		}
		default: {
			rb_raise(rb_eTypeError, "multiplicand's type is not supported");
		}
	}
	
	return Data_Wrap_Struct(cGMPRational, rational_mark, rational_free, r);
}

// Division (/)
// {GMP::Rational} -> {GMP::Rational}
VALUE
q_division( VALUE self, VALUE divisor ) {
	// Creates pointers to self's and the result's mpq_t structures
	mpq_t *r = malloc(sizeof(*r));
	mpq_t *q;
	
	// Loads self into *q
	Data_Get_Struct(self, mpq_t, q);
	
	// Inits the result
	mpq_init(*r);
	
	// Decides what to do based on the divisor's type/class
	switch (TYPE(divisor)) {
		case T_DATA: {
			if (rb_obj_class(divisor) == cGMPRational) {
				mpq_t *sr;
				Data_Get_Struct(divisor, mpq_t, sr);
				
				if (mpq_sgn(*sr) == 0)
					rb_raise(rb_eRuntimeError, "divided by zero");
				
				mpq_div(*r, *q, *sr);
			} else {
				rb_raise(rb_eTypeError, "divisor's type is not supported");
			}
			break;
		}
		default: {
			rb_raise(rb_eTypeError, "divisor's type is not supported");
		}
	}
	
	return Data_Wrap_Struct(cGMPRational, rational_mark, rational_free, r);
}
//// end of binary arithmetical operators
////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////
//// Unary arithmetical operators
// Plus (+a)
// {GMP::Rational} -> {GMP::Rational}
VALUE
q_positive( VALUE self ) {
	return self;
}

// Negation (-a)
// {GMP::Rational} -> {GMP::Rational}
VALUE
q_negation( VALUE self ) {
	// Creates pointers to self's and the result's mpq_t structures
	mpq_t *r = malloc(sizeof(*r));
	mpq_t *q;
	
	// Loads self into *q
	Data_Get_Struct(self, mpq_t, q);
	
	// Inits the result
	mpq_init(*r);
	
	// Negates i, and copies the result to r
	mpq_neg(*r, *q);
	
	return Data_Wrap_Struct(cGMPRational, rational_mark, rational_free, r);
}
//// end of unary arithmetical operators
////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////
//// Comparison methods
// Equality (==)
// {GMP::Rational} -> {TrueClass, FalseClass}
VALUE
q_equality_test( VALUE self, VALUE other ) {
	// Creates a mpq_t pointer and loads self in it
	mpq_t *q;
	Data_Get_Struct(self, mpq_t, q);
	
	// Immediate responses to avoid one object creation
	switch (TYPE(other)) {
		case T_DATA: {
			if (rb_obj_class(other) == cGMPRational) {
				mpq_t *od;
				Data_Get_Struct(other, mpq_t, od);
				
				if (mpq_cmp(*q, *od) == 0)
					return Qtrue;
				else
					return Qfalse;
			} else {
				rb_raise(rb_eTypeError, "input data type not supported");
			}
			break;
		}
		default: {
			rb_raise(rb_eTypeError, "input data type not supported");
		}
	}
}

// Greater than (>)
// {GMP::Rational} -> {TrueClass, FalseClass}
VALUE
q_greater_than_test( VALUE self, VALUE other ) {
	// Creates a mpq_t pointer and loads self in it
	mpq_t *q;
	Data_Get_Struct(self, mpq_t, q);
	
	// Immediate responses to avoid one object creation
	switch (TYPE(other)) {
		case T_DATA: {
			if (rb_obj_class(other) == cGMPRational) {
				mpq_t *od;
				Data_Get_Struct(other, mpq_t, od);
				
				if (mpq_cmp(*q, *od) > 0)
					return Qtrue;
				else
					return Qfalse;
			} else {
				rb_raise(rb_eTypeError, "input data type not supported");
			}
			break;
		}
		default: {
			rb_raise(rb_eTypeError, "input data type not supported");
		}
	}
}

// Less than (>)
// {GMP::Rational} -> {TrueClass, FalseClass}
VALUE
q_less_than_test( VALUE self, VALUE other ) {
	// Creates a mpq_t pointer and loads self in it
	mpq_t *q;
	Data_Get_Struct(self, mpq_t, q);
	
	// Immediate responses to avoid one object creation
	switch (TYPE(other)) {
		case T_DATA: {
			if (rb_obj_class(other) == cGMPRational) {
				mpq_t *od;
				Data_Get_Struct(other, mpq_t, od);
				
				if (mpq_cmp(*q, *od) < 0)
					return Qtrue;
				else
					return Qfalse;
			} else {
				rb_raise(rb_eTypeError, "input data type not supported");
			}
			break;
		}
		default: {
			rb_raise(rb_eTypeError, "input data type not supported");
		}
	}
}

// Greater than or equal to (>=)
// {GMP::Rational} -> {TrueClass, FalseClass}
VALUE
q_greater_than_or_equal_to_test( VALUE self, VALUE other ) {
	// Creates a mpq_t pointer and loads self in it
	mpq_t *q;
	Data_Get_Struct(self, mpq_t, q);
	
	// Immediate responses to avoid one object creation
	switch (TYPE(other)) {
		case T_DATA: {
			if (rb_obj_class(other) == cGMPRational) {
				mpq_t *od;
				Data_Get_Struct(other, mpq_t, od);
				
				if (mpq_cmp(*q, *od) >= 0)
					return Qtrue;
				else
					return Qfalse;
			} else {
				rb_raise(rb_eTypeError, "input data type not supported");
			}
			break;
		}
		default: {
			rb_raise(rb_eTypeError, "input data type not supported");
		}
	}
}

// Less than or equal to (<=)
// {GMP::Rational} -> {TrueClass, FalseClass}
VALUE
q_less_than_or_equal_to_test( VALUE self, VALUE other ) {
	// Creates a mpq_t pointer and loads self in it
	mpq_t *q;
	Data_Get_Struct(self, mpq_t, q);
	
	// Immediate responses to avoid one object creation
	switch (TYPE(other)) {
		case T_DATA: {
			if (rb_obj_class(other) == cGMPRational) {
				mpq_t *od;
				Data_Get_Struct(other, mpq_t, od);
				
				if (mpq_cmp(*q, *od) <= 0)
					return Qtrue;
				else
					return Qfalse;
			} else {
				rb_raise(rb_eTypeError, "input data type not supported");
			}
			break;
		}
		default: {
			rb_raise(rb_eTypeError, "input data type not supported");
		}
	}
}

// Generic comparison (<=>)
// {GMP::Rational} -> {Fixnum}
VALUE
q_generic_comparison( VALUE self, VALUE other ) {
	// Creates a mpq_t pointer and loads self in it
	mpq_t *q;
	Data_Get_Struct(self, mpq_t, q);
	
	// Immediate responses to avoid one object creation
	switch (TYPE(other)) {
		case T_DATA: {
			if (rb_obj_class(other) == cGMPRational) {
				mpq_t *od;
				Data_Get_Struct(other, mpq_t, od);
				
				return INT2FIX(mpq_cmp(*q, *od));
			} else {
				// I seriously need to start working on these error messages
				rb_raise(rb_eTypeError, "input data type not supported");
			}
			break;
		}
		default: {
			rb_raise(rb_eTypeError, "input data type not supported");
		}
	}
}
//// end of comparison operators
////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////
//// Numerator and denominator accessors
// Creates a new GMP::Integer object with the initial value of self's numerator
// {} -> {GMP::Integer}
VALUE
q_get_numerator( VALUE self, VALUE other ) {
	// Creates a mpq_t pointer and loads self into it.
	mpq_t *q;
	Data_Get_Struct(self, mpq_t, q);
	
	// Creates and initializes the result.
	mpz_t *z = malloc(sizeof(*z));
	mpz_init(*z);
	
	mpq_get_num(*z, *q);
	return Data_Wrap_Struct(cGMPInteger, integer_mark, integer_free, z);
}

// Creates a new GMP::Integer object with the initial value
// of self's denominator
// {} -> {GMP::Integer}
VALUE
q_get_denominator( VALUE self, VALUE other ) {
	// Creates a mpq_t pointer and loads self into it.
	mpq_t *q;
	Data_Get_Struct(self, mpq_t, q);
	
	// Creates and initializes the result.
	mpz_t *z = malloc(sizeof(*z));
	mpz_init(*z);
	
	mpq_get_den(*z, *q);
	return Data_Wrap_Struct(cGMPInteger, integer_mark, integer_free, z);
}
//// end of comparison operators
////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////
//// Other operations
// Efficient swap
// {GMP::Rational} -> {GMP::Rational}
VALUE
q_swap( VALUE self, VALUE other ) {
	// Creates pointers to self's and other's mpq_t structures
	mpq_t *q, *o;
	
	// Copies back the mpq_t pointers wrapped in ruby data objects
	Data_Get_Struct(self, mpq_t, q);
	Data_Get_Struct(other, mpq_t, o);
	
	// Swaps the contents of self and other
	mpq_swap(*q, *o);
	
	return Qnil;
}

// Sign
// {GMP::Rational} -> {Fixnum}
VALUE
q_sign( VALUE self ) {
	// Loads self into a mpq_t
	mpq_t *s;
	Data_Get_Struct(self, mpq_t, s);
	
	// Gets the sign of self
	int intSign = mpq_sgn(*s);
	
	return INT2FIX(intSign);
}

// Absolute value
// {} -> {GMP::Rational}
VALUE
q_absolute( VALUE self ) {
	// Creates pointers to self's and the result's mpq_t structures
	mpq_t *r = malloc(sizeof(*r));
	mpq_t *q;
	
	// Loads self into *q
	Data_Get_Struct(self, mpq_t, q);
	
	// Inits the result
	mpq_init(*r);
	
	// Sets the result as the absolute value of self
	mpq_abs(*r, *q);
	
	return Data_Wrap_Struct(cGMPRational, rational_mark, rational_free, r);
}

// Inversion (1/self)
// {} -> {GMP::Rational}
VALUE
q_invert( VALUE self ) {
	// Creates pointers to self's and the result's mpq_t structures
	mpq_t *r = malloc(sizeof(*r));
	mpq_t *q;
	
	// Loads self into *q
	Data_Get_Struct(self, mpq_t, q);
	
	// Raises an exception if the numerator is zero (which will make GMP
	// divide by zero).
	if (mpz_cmp_ui(mpq_numref(*q), 0) == 0)
		rb_raise(rb_eRuntimeError, "numerator cannot be zero");
	
	// Inits the result
	mpq_init(*r);
	
	// Sets the result as the absolute value of self
	mpq_inv(*r, *q);
	
	return Data_Wrap_Struct(cGMPRational, rational_mark, rational_free, r);
}
//// end of other operations
////////////////////////////////////////////////////////////////////



void
Init_gmpq() {
	// Defines the module GMP and class GMP::Rational
	mGMP = rb_define_module("GMP");
	cGMPInteger = rb_define_class_under(mGMP, "Integer", rb_cObject);
	cGMPRational = rb_define_class_under(mGMP, "Rational", rb_cObject);
	cGMPFloat = rb_define_class_under(mGMP, "Float", rb_cObject);
	
	// Book keeping and the constructor method
	rb_define_alloc_func(cGMPRational, rational_allocate);
	rb_define_method(cGMPRational, "initialize", q_init, 1);
	
	// Conversion methods
	rb_define_method(cGMPRational, "to_s", q_to_string, -1);
	rb_define_method(cGMPRational, "to_f", q_to_float, 0);
	rb_define_method(cGMPRational, "to_gmpf", q_to_gmpf, 0);
	
	// Binary arithmetical operators
	rb_define_method(cGMPRational, "+", q_addition, 1);
	rb_define_method(cGMPRational, "-", q_subtraction, 1);
	rb_define_method(cGMPRational, "*", q_multiplication, 1);
	rb_define_method(cGMPRational, "/", q_division, 1);
	
	// Unary arithmetical operators
	rb_define_method(cGMPRational, "+@", q_positive, 0);
	rb_define_method(cGMPRational, "-@", q_negation, 0);
	
	// Comparisons
	rb_define_method(cGMPRational, "==", q_equality_test, 1);
	rb_define_method(cGMPRational, ">", q_greater_than_test, 1);
	rb_define_method(cGMPRational, "<", q_less_than_test, 1);
	rb_define_method(cGMPRational, ">=", q_greater_than_or_equal_to_test, 1);
	rb_define_method(cGMPRational, "<=", q_less_than_or_equal_to_test, 1);
	rb_define_method(cGMPRational, "<=>", q_generic_comparison, 1);
	
	// Numerator and denominator accessors
	rb_define_method(cGMPRational, "num", q_get_numerator, 0);
	rb_define_method(cGMPRational, "den", q_get_denominator, 0);
//	rb_define_method(cGMPRational, "ref_num", q_getref_numerator, 0);
//	rb_define_method(cGMPRational, "ref_den", q_getref_denominator, 0);
	
	// Other operations
	rb_define_method(cGMPRational, "swap", q_swap, 1);
	rb_define_method(cGMPRational, "sign", q_sign, 0);
	rb_define_method(cGMPRational, "abs", q_absolute, 0);
	rb_define_method(cGMPRational, "inv", q_invert, 0);
}
