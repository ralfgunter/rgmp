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
integer_mark( mpz_t *i ) {}

void
integer_free( mpz_t *i ) {
	mpz_clear(*i);
}

// Object allocation
VALUE
integer_allocate( VALUE klass ) {
	mpz_t *i = malloc(sizeof(mpz_t));
	mpz_init(*i);
	return Data_Wrap_Struct(klass, integer_mark, integer_free, i);
}

// Class constructor
VALUE
z_init( VALUE self, VALUE intData ) {
	// Creates a mpz_t pointer and loads self in it
	mpz_t *i;
	Data_Get_Struct(self, mpz_t, i);
	
	switch (TYPE(intData)) {
		case T_STRING: {
			mpz_set_str(*i, StringValuePtr(intData), 10);
			break;
		}
		case T_FIXNUM: {
			mpz_set_si(*i, FIX2LONG(intData));
			break;
		}
		case T_DATA: {
			if (rb_obj_class(intData) == cGMPInteger) {
				mpz_t *gi;
				Data_Get_Struct(intData, mpz_t, gi);
				mpz_set(*i, *gi);
			} else {
				rb_raise(rb_eTypeError, "input data type not supported");
			}
			break;
		}
		case T_BIGNUM: {
			// I have a sort of bittersweet feeling about this one.
			// On the one hand, this more or less guarantees compatibility with
			// future Ruby versions by avoiding some internal ties with the way
			// Bignum is implemented, but on the other hand, it is not without
			// a (not yet measured) performance drop.
			VALUE str = rb_big2str(intData, 10);
			mpz_set_str(*i, StringValuePtr(str), 10);
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
//// Conversion methods (from C types to Ruby classes)
// To String
// {Fixnum} -> {String}
VALUE
z_to_string( VALUE argc, VALUE *argv, VALUE self ) {
	// Creates pointers for self and the final string
	mpz_t *s;
	Data_Get_Struct(self, mpz_t, s);
	
	// Creates a placeholder for the optional argument
	VALUE base;
	
	// The base argument is optional, and can vary from 2 to 62 and -2 to -36
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
	if ((intBase < 2 && intBase > -2) || (intBase < -36) || (intBase > 62))
		rb_raise(rb_eRangeError, "base out of range");
	
	char *intStr = mpz_get_str(NULL, intBase, *s);
	VALUE rStr = rb_str_new2(intStr);
	free(intStr);
	
	return rStr;
}

// To Fixnum/Bignum
VALUE
z_to_integer( VALUE self ) {
	// Creates a mpz_t pointer and loads self in it
	mpz_t *i;
	Data_Get_Struct(self, mpz_t, i);
	
	// See comments on 'case T_BIGNUM:' on the init function above
	if (mpz_cmp_ui(*i, FIXNUM_MAX) > 0 || mpz_cmp_ui(*i, FIXNUM_MIN) < 0) {
		return rb_cstr2inum(mpz_get_str(NULL, 10, *i), 10);
	} else {
		signed long tempLong = mpz_get_si(*i);
		return LONG2FIX(tempLong);
	}
}

// To Float (double-precision floating point number)
VALUE
z_to_float( VALUE self ) {
	// Creates a mpz_t pointer and loads self in it
	mpz_t *i;
	Data_Get_Struct(self, mpz_t, i);
	
	double tempDouble = mpz_get_d(*i);
	
	return rb_float_new(tempDouble);
}
//// end of conversion methods
////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////
//// Binary arithmetical operators (operations taking two values)
// Addition (+)
// {GMP::Integer, Fixnum, Bignum} -> {GMP::Integer}
VALUE
z_addition( VALUE self, VALUE summand ) {
	// Creates pointers to self's and the result's mpz_t structures
	mpz_t *r = malloc(sizeof(*r));
	mpz_t *i;
	
	// Loads self into *i
	Data_Get_Struct(self, mpz_t, i);
	
	// Inits the result
	mpz_init(*r);
	
	// Decides what to do based on the summand's type/class
	switch (TYPE(summand)) {
		case T_DATA: {
			if (rb_obj_class(summand) == cGMPInteger) {
				mpz_t *sd;
				Data_Get_Struct(summand, mpz_t, sd);
				mpz_add(*r, *i, *sd);
			} else {
				rb_raise(rb_eTypeError, "input data type not supported");
			}
			break;
		}
		case T_FIXNUM: {
			// Yep, also smells like a bad hack
			// Unfortunately, GMP does not have an addition function that deals
			// with signed ints
			mpz_t tempSuf;
			mpz_init_set_si(tempSuf, FIX2LONG(summand));
			mpz_add(*r, *i, tempSuf);
			mpz_clear(tempSuf);
			break;
		}
		case T_BIGNUM: {
			mpz_t tempSub;
			VALUE str = rb_big2str(summand, 10);
			mpz_init_set_str(tempSub, StringValuePtr(str), 10);
			mpz_add(*r, *i, tempSub);
			mpz_clear(tempSub);
			break;
		}
		default: {
			rb_raise(rb_eTypeError, "input data type not supported");
		}
	}
	
	return Data_Wrap_Struct(cGMPInteger, integer_mark, integer_free, r);
}

// Subtraction (-)
// {GMP::Integer, Fixnum, Bignum} -> {GMP::Integer}
VALUE
z_subtraction( VALUE self, VALUE subtraend ) {
	// Creates pointers to self's and the result's mpz_t structures
	mpz_t *r = malloc(sizeof(*r));
	mpz_t *i;
	
	// Loads self into *i
	Data_Get_Struct(self, mpz_t, i);
	
	// Inits the result
	mpz_init(*r);
	
	// Decides what to do based on the subtraend's type/class
	switch (TYPE(subtraend)) {
		case T_DATA: {
			if (rb_obj_class(subtraend) == cGMPInteger) {
				mpz_t *sd;
				Data_Get_Struct(subtraend, mpz_t, sd);
				mpz_sub(*r, *i, *sd);
			} else {
				rb_raise(rb_eTypeError, "input data type not supported");
			}
			break;
		}
		case T_FIXNUM: {
			// Yep, also smells like a bad hack
			// Unfortunately, GMP does not have a subtraction function that
			// deals with signed ints
			mpz_t tempSuf;
			mpz_init_set_si(tempSuf, FIX2LONG(subtraend));
			mpz_sub(*r, *i, tempSuf);
			break;
		}
		case T_BIGNUM: {
			mpz_t tempSub;
			VALUE str = rb_big2str(subtraend, 10);
			mpz_init_set_str(tempSub, StringValuePtr(str), 10);
			mpz_sub(*r, *i, tempSub);
			mpz_clear(tempSub);
			break;
		}
		default: {
			rb_raise(rb_eTypeError, "input data type not supported");
		}
	}
	
	return Data_Wrap_Struct(cGMPInteger, integer_mark, integer_free, r);
}

// Multiplication (*)
// {GMP::Integer, Fixnum, Bignum} -> {GMP::Integer}
VALUE
z_multiplication( VALUE self, VALUE multiplicand ) {
	// Creates pointers to self's and the result's mpz_t structures
	mpz_t *r = malloc(sizeof(*r));
	mpz_t *i;
	
	// Loads self into *i
	Data_Get_Struct(self, mpz_t, i);
	
	// Inits the result
	mpz_init(*r);
	
	// Decides what to do based on the multiplicand's type/class
	switch (TYPE(multiplicand)) {
		case T_DATA: {
			if (rb_obj_class(multiplicand) == cGMPInteger) {
				mpz_t *sd;
				Data_Get_Struct(multiplicand, mpz_t, sd);
				mpz_mul(*r, *i, *sd);
			} else {
				rb_raise(rb_eTypeError, "input data type not supported");
			}
			break;
		}
		case T_FIXNUM: {
			signed long sl = FIX2LONG(multiplicand);
			mpz_mul_si(*r, *i, sl);
			break;
		}
		case T_BIGNUM: {
			mpz_t tempMub;
			VALUE str = rb_big2str(multiplicand, 10);
			mpz_init_set_str(tempMub, StringValuePtr(str), 10);
			mpz_mul(*r, *i, tempMub);
			mpz_clear(tempMub);
			break;
		}
		default: {
			rb_raise(rb_eTypeError, "input data type not supported");
		}
	}
	
	return Data_Wrap_Struct(cGMPInteger, integer_mark, integer_free, r);
}

// Division (/)
// {GMP::Integer, Fixnum, Bignum} -> {GMP::Integer}
VALUE
z_division( VALUE self, VALUE dividend ) {
	// Creates pointers to self's and the result's mpz_t structures
	mpz_t *r = malloc(sizeof(*r));
	mpz_t *i;
	
	// Loads self into *i
	Data_Get_Struct(self, mpz_t, i);
	
	// Inits the result
	mpz_init(*r);
	
	// Decides what to do based on the dividend's type/class
	switch (TYPE(dividend)) {
		case T_DATA: {
			if (rb_obj_class(dividend) == cGMPInteger) {
				mpz_t *sd;
				Data_Get_Struct(dividend, mpz_t, sd);
				if (mpz_sgn(*sd) == 0)
					rb_raise(rb_eZeroDivError, "divided by 0");
				mpz_fdiv_q(*r, *i, *sd);
			} else {
				rb_raise(rb_eTypeError, "input data type not supported");
			}
			break;
		}
		case T_FIXNUM: {
			signed long sl = FIX2LONG(dividend);
			if (sl == 0)
				rb_raise(rb_eZeroDivError, "divided by 0");
			mpz_fdiv_q_ui(*r, *i, ((sl > 0) ? sl : -sl));
			if (sl < 0)
				mpz_neg(*r, *r);
			break;
		}
		case T_BIGNUM: {
			mpz_t tempDb;
			VALUE str = rb_big2str(dividend, 10);
			mpz_init_set_str(tempDb, StringValuePtr(str), 10);
			mpz_fdiv_q(*r, *i, tempDb);
			mpz_clear(tempDb);
			break;
		}
		default: {
			rb_raise(rb_eTypeError, "input data type not supported");
		}
	}
	
	return Data_Wrap_Struct(cGMPInteger, integer_mark, integer_free, r);
}

// Modulo (from modular arithmetic) (%)
// {GMP::Integer, Fixnum} -> {GMP::Integer}
VALUE
z_modulo( VALUE self, VALUE base ) {
	// Creates pointers to self's and the result's mpz_t structures
	mpz_t *r = malloc(sizeof(*r));
	mpz_t *i;
	
	// Loads self into *i
	Data_Get_Struct(self, mpz_t, i);
	
	// Inits the result
	mpz_init(*r);
	
	// Decides what to do based on the base's type/class
	switch (TYPE(base)) {
		case T_DATA: {
			if (rb_obj_class(base) == cGMPInteger) {
				mpz_t *sd;
				Data_Get_Struct(base, mpz_t, sd);
				// Ensures the base is not zero before doing the math
				if (mpz_cmp_ui(*sd, (unsigned long) 0) == 0)
					rb_raise(rb_eRuntimeError, "base cannot be zero");
				mpz_mod(*r, *i, *sd);
			} else {
				rb_raise(rb_eTypeError, "input data type not supported");
			}
			break;
		}
		case T_FIXNUM: {
			unsigned long sl = FIX2LONG(base);
			// Ensures the base is not zero before doing the math
			if (sl == (unsigned long) 0)
				rb_raise(rb_eRuntimeError, "base cannot be zero");
			mpz_mod_ui(*r, *i, sl);
			break;
		}
		default: {
			rb_raise(rb_eTypeError, "input data type not supported");
		}
	}
	
	return Data_Wrap_Struct(cGMPInteger, integer_mark, integer_free, r);
}

// Exponetiation (**)
// {GMP::Integer, Fixnum} -> {GMP::Integer}
VALUE
z_power( VALUE self, VALUE exp ) {
	// Creates pointers to self's and the result's mpz_t structures
	mpz_t *r = malloc(sizeof(*r));
	mpz_t *i;
	
	// Loads self into *i
	Data_Get_Struct(self, mpz_t, i);
	
	// Inits the result
	mpz_init(*r);
	
	// Decides what to do based on the exponent's type/class
	switch (TYPE(exp)) {
		case T_DATA: {
			if (rb_obj_class(exp) == cGMPInteger) {
				// Unfortunately GMP does not provide a pure exponetiation
				// method that takes mpz_t exponents, so we have to convert it
				// to an unsigned long first, and then use call mpz_pow_ui to
				// get the result.
				mpz_t *sd;
				Data_Get_Struct(exp, mpz_t, sd);
				
				if (mpz_cmp_ui(*sd, ULONG_MAX) > 0)
					rb_raise(rb_eRangeError, "exponent must fit in a ulong");
				
				if (mpz_cmp_ui(*sd, 0) < 0)
					rb_raise(rb_eRangeError, "exponent must be non-negative");
				
				mpz_pow_ui(*r, *i, mpz_get_ui(*sd));
			} else {
				rb_raise(rb_eTypeError, "input data type not supported");
			}
			break;
		}
		case T_FIXNUM: {
			unsigned long sl = FIX2LONG(exp);
			if ((signed long) sl < 0)
				rb_raise(rb_eRangeError, "exponent must be non-negative");
			mpz_pow_ui(*r, *i, sl);
			break;
		}
		default: {
			rb_raise(rb_eTypeError, "exponent's type not supported");
		}
	}
	
	return Data_Wrap_Struct(cGMPInteger, integer_mark, integer_free, r);
}

// Left shift (also multiplication by a power of 2)
// {Fixnum, Bignum} -> {GMP::Integer}
VALUE
z_left_shift( VALUE self, VALUE shift ) {
	// Creates pointers to self's and the result's mpz_t structures
	// Also creates a placeholder for the shift amount
	unsigned long longShift;	// No pun intended
	mpz_t *r = malloc(sizeof(*r));
	mpz_t *i;
	
	// Loads self into *i
	Data_Get_Struct(self, mpz_t, i);
	
	// Inits the result
	mpz_init(*r);
	
	// If shift is of the correct type, then do the job!
	// TODO: it is debatable whether or not we should check for
	// the sign of the shift argument. If the overhead is effectively
	// negligible, then we should do it.
	if (!(FIXNUM_P(shift) || TYPE(shift) == T_BIGNUM))
		rb_raise(rb_eTypeError, "shift is not of a supported type");
	longShift = NUM2LONG(shift);
	mpz_mul_2exp(*r, *i, longShift);
	
	return Data_Wrap_Struct(cGMPInteger, integer_mark, integer_free, r);
}

// Right shift (also division by a power of 2)
// {Fixnum, Bignum} -> {GMP::Integer}
VALUE
z_right_shift( VALUE self, VALUE shift ) {
	// Creates pointers to self's and the result's mpz_t structures
	// Also creates a placeholder for the shift amount
	unsigned long longShift;	// No pun intended
	mpz_t *r = malloc(sizeof(*r));
	mpz_t *i;
	
	// Loads self into *i
	Data_Get_Struct(self, mpz_t, i);
	
	// Inits the result
	mpz_init(*r);
	
	// If shift is of the correct type, then do the job!
	// TODO: it is debatable whether or not we should check for
	// the sign of the shift argument. If the overhead is effectively
	// negligible, then we should do it.
	if (!(FIXNUM_P(shift) || TYPE(shift) == T_BIGNUM))
		rb_raise(rb_eTypeError, "shift is not of a supported type");
	longShift = NUM2LONG(shift);
	mpz_fdiv_q_2exp(*r, *i, longShift);
	
	return Data_Wrap_Struct(cGMPInteger, integer_mark, integer_free, r);
}
//// end of binary operator methods
////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////
//// Unary arithmetical operators (operations over a single value)
// Plus (+a)
// {GMP::Integer} -> {GMP::Integer}
VALUE
z_positive( VALUE self ) {
	return self;
}

// Negation (-a)
// {GMP::Integer} -> {GMP::Integer}
VALUE
z_negation( VALUE self ) {
	// Creates pointers to self's and the result's mpz_t structures
	mpz_t *r = malloc(sizeof(*r));
	mpz_t *i;
	
	// Loads self into *i
	Data_Get_Struct(self, mpz_t, i);
	
	// Inits the result
	mpz_init(*r);
	
	// Negates i, and copies the result to r
	mpz_neg(*r, *i);
	
	return Data_Wrap_Struct(cGMPInteger, integer_mark, integer_free, r);
}
//// end of unary operators methods
////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////
//// Logic operators
// Logic AND (&)
// {GMP::Integer, Fixnum} -> {GMP::Integer}
VALUE
z_logic_and( VALUE self, VALUE other ) {
	// Creates pointers to self's and result's mpz_t structures
	mpz_t *r = malloc(sizeof(*r));
	mpz_t *i;
	
	// Loads self from Ruby
	Data_Get_Struct(self, mpz_t, i);
	
	// This might be a little faster than the previous method, since only
	// two classes are present.
	if (rb_obj_class(other) == cGMPInteger) {
		mpz_t *oz;
		Data_Get_Struct(other, mpz_t, oz);
		
		mpz_init(*r);
		mpz_and(*r, *i, *oz);
	} else if (FIXNUM_P(other)) {
		// GMP's logic operators only accept mpz_t as arguments, therefore
		// we have to create a temporary mpz_t to hold 'other'.
		signed long ol = FIX2LONG(other);
		mpz_t olz;
		mpz_init_set_si(olz, ol);
		
		mpz_init(*r);
		mpz_and(*r, *i, olz);
		mpz_clear(olz);
	}
	
	return Data_Wrap_Struct(cGMPInteger, integer_mark, integer_free, r);
}

// Logic OR (inclusive OR) (|)
// {GMP::Integer, Fixnum} -> {GMP::Integer}
VALUE
z_logic_ior( VALUE self, VALUE other ) {
	// Creates pointers to self's and result's mpz_t structures
	mpz_t *r = malloc(sizeof(*r));
	mpz_t *i;
	
	// Loads self from Ruby
	Data_Get_Struct(self, mpz_t, i);
	
	// This might be a little faster than the previous method, since only
	// two classes are present.
	if (rb_obj_class(other) == cGMPInteger) {
		mpz_t *oz;
		Data_Get_Struct(other, mpz_t, oz);
		
		mpz_init(*r);
		mpz_ior(*r, *i, *oz);
	} else if (FIXNUM_P(other)) {
		// GMP's logic operators only accept mpz_t as arguments, therefore
		// we have to create a temporary mpz_t to hold 'other'.
		signed long ol = FIX2LONG(other);
		mpz_t olz;
		mpz_init_set_si(olz, ol);
		
		mpz_init(*r);
		mpz_ior(*r, *i, olz);
		mpz_clear(olz);
	}
	
	return Data_Wrap_Struct(cGMPInteger, integer_mark, integer_free, r);
}

// Logic XOR (exclusive OR) (^)
// {GMP::Integer, Fixnum} -> {GMP::Integer}
VALUE 
z_logic_xor( VALUE self, VALUE other ) {
	// Creates pointers to self's and result's mpz_t structures
	mpz_t *r = malloc(sizeof(*r));
	mpz_t *i;
	
	// Loads self from Ruby
	Data_Get_Struct(self, mpz_t, i);
	
	// This might be a little faster than the previous method, since only
	// two classes are present.
	if (rb_obj_class(other) == cGMPInteger) {
		mpz_t *oz;
		Data_Get_Struct(other, mpz_t, oz);
		
		mpz_init(*r);
		mpz_xor(*r, *i, *oz);
	} else if (FIXNUM_P(other)) {
		// GMP's logic operators only accept mpz_t as arguments, therefore
		// we have to create a temporary mpz_t to hold 'other'.
		signed long ol = FIX2LONG(other);
		mpz_t olz;
		mpz_init_set_si(olz, ol);
		
		mpz_init(*r);
		mpz_xor(*r, *i, olz);
		mpz_clear(olz);
	}
	
	return Data_Wrap_Struct(cGMPInteger, integer_mark, integer_free, r);
}

// Logic NOT (~)
// {GMP::Integer} -> {GMP::Integer}
VALUE
z_logic_not( VALUE self ) {
	// Creates pointers to self's and the result's mpz_t structures
	mpz_t *r = malloc(sizeof(*r));
	mpz_t *i;
	
	// Loads self into *i
	Data_Get_Struct(self, mpz_t, i);
	
	// Inits the result
	mpz_init(*r);
	
	// Sets result as the one's complement of self
	mpz_com(*r, *i);
	
	return Data_Wrap_Struct(cGMPInteger, integer_mark, integer_free, r);
}
//// end of logic manipulation methods
////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////
//// Comparison methods
// Equality (==)
// {GMP::Integer} -> {TrueClass, FalseClass}
VALUE
z_equality_test( VALUE self, VALUE other ) {
	// Creates a mpz_t pointer and loads self in it
	mpz_t *i;
	Data_Get_Struct(self, mpz_t, i);
	
	// Immediate responses to avoid one object creation
	switch (TYPE(other)) {
		case T_DATA: {
			if (rb_obj_class(other) == cGMPInteger) {
				mpz_t *od;
				Data_Get_Struct(other, mpz_t, od);
				
				if (mpz_cmp(*i, *od) == 0)
					return Qtrue;
				else
					return Qfalse;
			} else {
				rb_raise(rb_eTypeError, "input data type not supported");
			}
			break;
		}
		case T_FIXNUM: {
			signed long ol = FIX2LONG(other);
			
			if (mpz_cmp_si(*i, ol) == 0)
				return Qtrue;
			else
				return Qfalse;
			
			break;
		}
		case T_BIGNUM: {
			mpz_t tempOb;
			VALUE str = rb_big2str(other, 10);
			mpz_init_set_str(tempOb, StringValuePtr(str), 10);
			
			if (mpz_cmp(*i, tempOb) == 0) {
				mpz_clear(tempOb);
				return Qtrue;
			} else {
				mpz_clear(tempOb);
				return Qfalse;
			}
			
			break;
		}
		default: {
			rb_raise(rb_eTypeError, "input data type not supported");
		}
	}
	
	// I'm guessing no compilers will complain about the lack of this?
	// Anyhow, just in case...
	return Qfalse;
}

// Greater than (>)
// {GMP::Integer} -> {TrueClass, FalseClass}
VALUE
z_greater_than_test( VALUE self, VALUE other ) {
	// Creates a mpz_t pointer and loads self in it
	mpz_t *i;
	Data_Get_Struct(self, mpz_t, i);
	
	// Immediate responses to avoid one object creation
	switch (TYPE(other)) {
		case T_DATA: {
			if (rb_obj_class(other) == cGMPInteger) {
				mpz_t *od;
				Data_Get_Struct(other, mpz_t, od);
				
				if (mpz_cmp(*i, *od) > 0)
					return Qtrue;
				else
					return Qfalse;
			} else {
				rb_raise(rb_eTypeError, "input data type not supported");
			}
			break;
		}
		case T_FIXNUM: {
			signed long ol = FIX2LONG(other);
			if (mpz_cmp_si(*i, ol) > 0)
				return Qtrue;
			else
				return Qfalse;
			break;
		}
		case T_BIGNUM: {
			mpz_t tempOb;
			VALUE str = rb_big2str(other, 10);
			mpz_init_set_str(tempOb, StringValuePtr(str), 10);
			if (mpz_cmp(*i, tempOb) > 0) {
				mpz_clear(tempOb);
				return Qtrue;
			} else {
				mpz_clear(tempOb);
				return Qfalse;
			}
			break;
		}
		default: {
			rb_raise(rb_eTypeError, "input data type not supported");
		}
	}
	
	// I'm guessing no compilers will complain about the lack of this?
	// Anyhow, just in case...
	return Qfalse;
}

// Less than (<)
// {GMP::Integer} -> {TrueClass, FalseClass}
VALUE
z_less_than_test( VALUE self, VALUE other ) {
	// Creates a mpz_t pointer and loads self in it
	mpz_t *i;
	Data_Get_Struct(self, mpz_t, i);
	
	// Immediate responses to avoid one object creation
	switch (TYPE(other)) {
		case T_DATA: {
			if (rb_obj_class(other) == cGMPInteger) {
				mpz_t *od;
				Data_Get_Struct(other, mpz_t, od);
				
				if (mpz_cmp(*i, *od) < 0)
					return Qtrue;
				else
					return Qfalse;
			} else {
				rb_raise(rb_eTypeError, "input data type not supported");
			}
			break;
		}
		case T_FIXNUM: {
			signed long ol = FIX2LONG(other);
			if (mpz_cmp_si(*i, ol) < 0)
				return Qtrue;
			else
				return Qfalse;
			break;
		}
		case T_BIGNUM: {
			mpz_t tempOb;
			VALUE str = rb_big2str(other, 10);
			mpz_init_set_str(tempOb, StringValuePtr(str), 10);
			if (mpz_cmp(*i, tempOb) < 0) {
				mpz_clear(tempOb);
				return Qtrue;
			} else {
				mpz_clear(tempOb);
				return Qfalse;
			}
			break;
		}
		default: {
			rb_raise(rb_eTypeError, "input data type not supported");
		}
	}
	
	// I'm guessing no compilers will complain about the lack of this?
	// Anyhow, just in case...
	return Qfalse;
}

// Greater than or equal to (>=)
// {GMP::Integer} -> {TrueClass, FalseClass}
VALUE
z_greater_than_or_equal_to_test( VALUE self, VALUE other ) {
	// Creates a mpz_t pointer and loads self in it
	mpz_t *i;
	Data_Get_Struct(self, mpz_t, i);
	
	// Immediate responses to avoid one object creation
	switch (TYPE(other)) {
		case T_DATA: {
			if (rb_obj_class(other) == cGMPInteger) {
				mpz_t *od;
				Data_Get_Struct(other, mpz_t, od);
				
				if (mpz_cmp(*i, *od) >= 0)
					return Qtrue;
				else
					return Qfalse;
			} else {
				rb_raise(rb_eTypeError, "input data type not supported");
			}
			break;
		}
		case T_FIXNUM: {
			signed long ol = FIX2LONG(other);
			if (mpz_cmp_si(*i, ol) >= 0)
				return Qtrue;
			else
				return Qfalse;
			break;
		}
		case T_BIGNUM: {
			mpz_t tempOb;
			VALUE str = rb_big2str(other, 10);
			mpz_init_set_str(tempOb, StringValuePtr(str), 10);
			if (mpz_cmp(*i, tempOb) >= 0) {
				mpz_clear(tempOb);
				return Qtrue;
			} else {
				mpz_clear(tempOb);
				return Qfalse;
			}
			break;
		}
		default: {
			rb_raise(rb_eTypeError, "input data type not supported");
		}
	}
	
	// I'm guessing no compilers will complain about the lack of this?
	// Anyhow, just in case...
	return Qfalse;
}

// Less than or equal to (<=)
// {GMP::Integer} -> {TrueClass, FalseClass}
VALUE
z_less_than_or_equal_to_test( VALUE self, VALUE other ) {
	// Creates a mpz_t pointer and loads self in it
	mpz_t *i;
	Data_Get_Struct(self, mpz_t, i);
	
	// Immediate responses to avoid one object creation
	switch (TYPE(other)) {
		case T_DATA: {
			if (rb_obj_class(other) == cGMPInteger) {
				mpz_t *od;
				Data_Get_Struct(other, mpz_t, od);
				
				if (mpz_cmp(*i, *od) <= 0)
					return Qtrue;
				else
					return Qfalse;
			} else {
				rb_raise(rb_eTypeError, "input data type not supported");
			}
			break;
		}
		case T_FIXNUM: {
			signed long ol = FIX2LONG(other);
			if (mpz_cmp_si(*i, ol) <= 0)
				return Qtrue;
			else
				return Qfalse;
			break;
		}
		case T_BIGNUM: {
			mpz_t tempOb;
			VALUE str = rb_big2str(other, 10);
			mpz_init_set_str(tempOb, StringValuePtr(str), 10);
			if (mpz_cmp(*i, tempOb) <= 0) {
				mpz_clear(tempOb);
				return Qtrue;
			} else {
				mpz_clear(tempOb);
				return Qfalse;
			}
			break;
		}
		default: {
			rb_raise(rb_eTypeError, "input data type not supported");
		}
	}
	
	// I'm guessing no compilers will complain about the lack of this?
	// Anyhow, just in case...
	return Qfalse;
}

// Generic comparison (<=>)
// {GMP::Integer} -> {Fixnum}
VALUE
z_generic_comparison( VALUE self, VALUE other ) {
	// Creates a mpz_t pointer and loads self in it
	mpz_t *i;
	Data_Get_Struct(self, mpz_t, i);
	
	// Immediate responses to avoid one object creation
	switch (TYPE(other)) {
		case T_DATA: {
			if (rb_obj_class(other) == cGMPInteger) {
				mpz_t *od;
				Data_Get_Struct(other, mpz_t, od);
				return INT2FIX(mpz_cmp(*i, *od));
			} else {
				rb_raise(rb_eTypeError, "input data type not supported");
			}
			break;
		}
		case T_FIXNUM: {
			signed long ol = FIX2LONG(other);
			return INT2FIX(mpz_cmp_si(*i, ol));
			break;
		}
		case T_BIGNUM: {
			mpz_t tempOb;
			VALUE str = rb_big2str(other, 10);
			mpz_init_set_str(tempOb, StringValuePtr(str), 10);
			return INT2FIX(mpz_cmp(*i, tempOb));
			break;
		}
		default: {
			rb_raise(rb_eTypeError, "input data type not supported");
		}
	}
	
	// I'm guessing no compilers will complain about the lack of this?
	// Anyhow, just in case...
	return Qfalse;
}

//// end of comparison methods
////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////
//// Inplace methods (the caller inherits the result)
// (probably) Sets self as the next prime greater than itself
// {} -> {GMP::Integer}
VALUE
z_next_prime_inplace( VALUE self ) {
	// Creates a mpz_t pointer and loads self in it
	mpz_t *i;
	Data_Get_Struct(self, mpz_t, i);
	
	mpz_nextprime(*i, *i);
	
	return Qnil;
}

// Absolute value
// {} -> {GMP::Integer}
VALUE
z_absolute_inplace( VALUE self ) {
	// Creates a mpz_t pointer and loads self in it
	mpz_t *i;
	Data_Get_Struct(self, mpz_t, i);
	
	mpz_abs(*i, *i);
	
	return Qnil;
}

// Negation
// {GMP::Integer} -> {GMP::Integer}
VALUE
z_negation_inplace( VALUE self ) {
	// Creates a mpz_t pointer and loads self in it
	mpz_t *i;
	Data_Get_Struct(self, mpz_t, i);
	
	mpz_neg(*i, *i);
	
	return Qnil;
}

// Square root (instance method)
// {} -> {GMP::Integer}
VALUE
z_sqrt_inplace( VALUE self ) {
	// Creates a mpz_t pointer and loads self in it
	mpz_t *i;
	Data_Get_Struct(self, mpz_t, i);
	
	mpz_sqrt(*i, *i);
	
	return Qnil;
}

// Nth root (instance method)
// {Fixnum, Bignum} -> {NilClass}
VALUE
z_root_inplace( VALUE self, VALUE degree ) {
	// Creates a mpz_t pointer and loads self in it
	// Also loads degree into an unsigned long
	mpz_t *i;
	unsigned long longDegree = NUM2LONG(degree);
	Data_Get_Struct(self, mpz_t, i);
	
	// If the degree is zero, GMP will normally give a floating point error
	// A possible solution is to make the degree -1, which works as expected
	if (longDegree == (unsigned long) 0)
		--longDegree;
	mpz_root(*i, *i, longDegree);
	
	return Qnil;
}

// Inversion (number theory; a*ã == 1 (mod m))
// {GMP::Integer} -> {NilClass}
VALUE
z_invert_inplace( VALUE self, VALUE base ) {
	// Creates pointers for self's and base's mpz_t structures
	// Also creates an int which will be used to check if the number has an
	// inverse on that base
	mpz_t *i, *b;
	int check;
	
	// Copies back the mpz_t pointers wrapped in ruby data objects
	Data_Get_Struct(self, mpz_t, i);
	Data_Get_Struct(base, mpz_t, b);
	
	check = mpz_invert(*i, *i, *b);
	
	if (check == 0)
		rb_raise(rb_eRuntimeError, "input is not invertible on this base");
	
	return Qnil;
}

// Setting a specific bit
// {Fixnum}, {Fixnum} -> {NilClass}
VALUE
z_set_bit_inplace( VALUE self, VALUE index, VALUE newValue ) {
	// Creates a pointer for self's mpz_t structure and two unsigned long
	// for the bit index and desired bit value
	mpz_t *i;
	unsigned long longIndex;
	int intNewValue;
	
	// Checks if the arguments are of the correct type
	if (!FIXNUM_P(newValue) || !(FIXNUM_P(index)))
		rb_raise(rb_eTypeError, "inputs are of the wrong type");
	
	// Loads and checks if the bit is in fact a bit
	intNewValue = FIX2INT(newValue);
	if (intNewValue != 0 && intNewValue != 1)
		rb_raise(rb_eTypeError, "new bit value is not a bit");
	
	// Loads and checks if the index is within range.
	// Horrible hack: the first NUM2LONG messes up negative indexes
	// while the second one does consider them.
	longIndex = FIX2LONG(index);
	if (longIndex < 0)
		rb_raise(rb_eRangeError, "bit position out of range");
	
	// Copies back the mpz_t pointer wrapped in a ruby data object
	Data_Get_Struct(self, mpz_t, i);
	
	// Sets the bit accordingly
	if (mpz_tstbit(*i, longIndex) != intNewValue)
		mpz_combit(*i, longIndex);
	
	return Qnil;
}

// Addition
// {GMP::Integer, Fixnum, Bignum} -> {}
VALUE
z_addition_inplace( VALUE self, VALUE summand ) {
	// Creates a mpz_t pointer and loads self in it
	mpz_t *i;
	Data_Get_Struct(self, mpz_t, i);
	
	switch (TYPE(summand)) {
		case T_DATA: {
			if (rb_obj_class(summand) == cGMPInteger) {
				mpz_t *sd;
				Data_Get_Struct(summand, mpz_t, sd);
				mpz_add(*i, *i, *sd);
			} else {
				rb_raise(rb_eTypeError, "input data type not supported");
			}
			break;
		}
		case T_FIXNUM: {
			// Yep, also smells like a bad hack
			// Unfortunately, GMP does not have an addition function that deals
			// with signed ints
			mpz_t tempSuf;
			mpz_init_set_si(tempSuf, FIX2LONG(summand));
			mpz_add(*i, *i, tempSuf);
			mpz_clear(tempSuf);
			break;
		}
		case T_BIGNUM: {
			mpz_t tempSub;
			VALUE str = rb_big2str(summand, 10);
			mpz_init_set_str(tempSub, StringValuePtr(str), 10);
			mpz_add(*i, *i, tempSub);
			mpz_clear(tempSub);
			break;
		}
		default: {
			rb_raise(rb_eTypeError, "input data type not supported");
		}
	}
	
	return Qnil;
}

// Subtraction
// {GMP::Integer, Fixnum, Bignum} -> {}
VALUE
z_subtraction_inplace( VALUE self, VALUE subtraend ) {
	// Creates a mpz_t pointer and loads self in it
	mpz_t *i;
	Data_Get_Struct(self, mpz_t, i);
	
	switch (TYPE(subtraend)) {
		case T_DATA: {
			if (rb_obj_class(subtraend) == cGMPInteger) {
				mpz_t *sd;
				Data_Get_Struct(subtraend, mpz_t, sd);
				mpz_sub(*i, *i, *sd);
			} else {
				rb_raise(rb_eTypeError, "input data type not supported");
			}
			break;
		}
		case T_FIXNUM: {
			// Yep, also smells like a bad hack
			// Unfortunately, GMP does not have a subtraction function that
			// deals with signed ints
			mpz_t tempSuf;
			mpz_init_set_si(tempSuf, FIX2LONG(subtraend));
			mpz_sub(*i, *i, tempSuf);
			break;
		}
		case T_BIGNUM: {
			mpz_t tempSub;
			VALUE str = rb_big2str(subtraend, 10);
			mpz_init_set_str(tempSub, StringValuePtr(str), 10);
			mpz_sub(*i, *i, tempSub);
			mpz_clear(tempSub);
			break;
		}
		default: {
			rb_raise(rb_eTypeError, "input data type not supported");
		}
	}
	
	return Qnil;
}

// Multiplication
// {GMP::Integer, Fixnum, Bignum} -> {}
VALUE
z_multiplication_inplace( VALUE self, VALUE multiplicand ) {
	// Creates a mpz_t pointer and loads self in it
	mpz_t *i;
	Data_Get_Struct(self, mpz_t, i);
		
	// Decides what to do based on the multiplicand's type/class
	switch (TYPE(multiplicand)) {
		case T_DATA: {
			if (rb_obj_class(multiplicand) == cGMPInteger) {
				mpz_t *sd;
				Data_Get_Struct(multiplicand, mpz_t, sd);
				mpz_mul(*i, *i, *sd);
			} else {
				rb_raise(rb_eTypeError, "input data type not supported");
			}
			break;
		}
		case T_FIXNUM: {
			signed long sl = FIX2LONG(multiplicand);
			mpz_mul_si(*i, *i, sl);
			break;
		}
		case T_BIGNUM: {
			mpz_t tempMub;
			VALUE str = rb_big2str(multiplicand, 10);
			mpz_init_set_str(tempMub, StringValuePtr(str), 10);
			mpz_mul(*i, *i, tempMub);
			mpz_clear(tempMub);
			break;
		}
		default: {
			rb_raise(rb_eTypeError, "input data type not supported");
		}
	}
	
	return Qnil;
}

// Multiply two values and then add to self
// {GMP::Integer}, {GMP::Integer}
VALUE
z_addmul_inplace( VALUE self, VALUE first, VALUE second ) {
	// Create pointers to self, first and second
	mpz_t *i, *f, *s;
	
	// Loads all three from Ruby
	Data_Get_Struct(second, mpz_t, s);
	Data_Get_Struct(first, mpz_t, f);
	Data_Get_Struct(self, mpz_t, i);
	
	// Does the calculation
	mpz_addmul(*i, *f, *s);
	
	return Qnil;
}

// Multiply two values and then subtract from self
// {GMP::Integer}, {GMP::Integer}
VALUE
z_submul_inplace( VALUE self, VALUE first, VALUE second ) {
	// Create pointers to self, first and second
	mpz_t *i, *f, *s;
	
	// Loads all three from Ruby
	Data_Get_Struct(second, mpz_t, s);
	Data_Get_Struct(first, mpz_t, f);
	Data_Get_Struct(self, mpz_t, i);
	
	// Does the calculation
	mpz_submul(*i, *f, *s);
	
	return Qnil;
}
//// end of inplace methods
////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////
//// Question-like methods
// Does 'base' divide 'self'?
// {GMP::Integer, Fixnum} -> {TrueClass, FalseClass}
VALUE
z_divisible( VALUE self, VALUE base ) {
	// Creates a mpz_t pointer and loads self in it
	mpz_t *i;
	Data_Get_Struct(self, mpz_t, i);
	
	// Placeholder for the result (either Qtrue or Qfalse)
	// TODO: check whether this gives a performance penalty
	// in comparison to returning from within the case block
	VALUE result;
	
	switch (TYPE(base)) {
		case T_DATA: {
			mpz_t *sd;
			Data_Get_Struct(base, mpz_t, sd);
			
			if (! mpz_divisible_p(*i, *sd))
				result = Qfalse;
			else
				result = Qtrue;
			break;
		}
		case T_FIXNUM: {
			unsigned long sl = FIX2LONG(base);
			if (! mpz_divisible_ui_p(*i, sl))
				result = Qfalse;
			else
				result = Qtrue;
			break;
		}
		default: {
			rb_raise(rb_eTypeError, "input data type not supported");
		}
	}
	
	return result;
}

VALUE
z_perfect_power( VALUE self ) {
	// Creates a mpz_t pointer and loads self in it
	mpz_t *i;
	Data_Get_Struct(self, mpz_t, i);
	
	// As we do not need a switch/case statement here, let's just
	// return stuff directly within the if block, for performance's
	// and brevity's sake
	if (! mpz_perfect_power_p(*i))
		return Qfalse;
	else
		return Qtrue;
}

VALUE
z_perfect_square( VALUE self ) {
	// Creates a mpz_t pointer and loads self in it
	mpz_t *i;
	Data_Get_Struct(self, mpz_t, i);
	
	// As we do not need a switch/case statement here, let's just
	// return stuff directly within the if block, for performance's
	// and brevity's sake
	if (! mpz_perfect_square_p(*i))
		return Qfalse;
	else
		return Qtrue;
}

VALUE
z_probable_prime( VALUE self, VALUE numberOfTests ) {
	// Creates a mpz_t pointer and loads self in it
	mpz_t *i;
	Data_Get_Struct(self, mpz_t, i);
	
	// Converts numberOfTests from Fixnum to int
	int numTests = FIX2INT(numberOfTests);
	
	// As we do not need a switch/case statement here, let's just
	// return stuff directly within the if block, for performance's
	// and brevity's sake
	int testResult = mpz_probab_prime_p(*i, numTests);
	if ( testResult == 0 ) 
		return Qfalse;
	else if ( testResult == 1 )
		return Qnil;
	else
		return Qtrue;
}

VALUE
z_even( VALUE self ) {
	// Creates a mpz_t pointer and loads self in it
	mpz_t *i;
	Data_Get_Struct(self, mpz_t, i);
	
	// As we do not need a switch/case statement here, let's just
	// return stuff directly within the if block, for performance's
	// and brevity's sake
	if (mpz_even_p(*i))
		return Qtrue;
	else
		return Qfalse;
}

VALUE
z_odd( VALUE self ) {
	// Creates a mpz_t pointer and loads self in it
	mpz_t *i;
	Data_Get_Struct(self, mpz_t, i);
	
	// As we do not need a switch/case statement here, let's just
	// return stuff directly within the if block, for performance's
	// and brevity's sake
	if (mpz_odd_p(*i))
		return Qtrue;
	else
		return Qfalse;
}

// Precise equality (checks the class as well)
// {GMP::Integer} -> {TrueClass, FalseClass}
VALUE
z_precise_equality( VALUE self, VALUE other ) {
	// Makes sure other's class is also GMP::Integer
	if (! rb_obj_class(other) == cGMPInteger)
		return Qfalse;
	
	// Creates pointers to self's and the other's mpz_t structures
	mpz_t *i, *o;
	
	// Copies back the mpz_t pointers wrapped in ruby data objects
	Data_Get_Struct(self, mpz_t, i);
	Data_Get_Struct(other, mpz_t, o);
	
	if (mpz_cmp(*i, *o) == 0)
		return Qtrue;
	else
		return Qfalse;
}

// Is it zero?
// {} -> {TrueClass, FalseClass}
VALUE
z_zero( VALUE self ) {
	// Creates a mpz_t pointer and loads self in it
	mpz_t *i;
	Data_Get_Struct(self, mpz_t, i);
	
	if (mpz_sgn(*i) == 0)
		return Qtrue;
	else
		return Qfalse;
}

// Or is it not zero?
// {} -> {TrueClass, FalseClass}
VALUE
z_nonzero( VALUE self ) {
	// Creates a mpz_t pointer and loads self in it
	mpz_t *i;
	Data_Get_Struct(self, mpz_t, i);
	
	if (mpz_sgn(*i) != 0)
		return Qtrue;
	else
		return Qfalse;
}
//// end of question-like methods
////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////
//// Other operations
// Absolute value
// {} -> {GMP::Integer}
VALUE
z_absolute( VALUE self ) {
	// Creates pointers to self's and the result's mpz_t structures
	mpz_t *r = malloc(sizeof(*r));
	mpz_t *i;
	
	// Loads self into *i
	Data_Get_Struct(self, mpz_t, i);
	
	// Inits the result
	mpz_init(*r);
	
	// Sets the result as the absolute value of self
	mpz_abs(*r, *i);
	
	return Data_Wrap_Struct(cGMPInteger, integer_mark, integer_free, r);
}

VALUE
z_next_prime( VALUE self ) {
	// Creates pointers to self's and the result's mpz_t structures
	mpz_t *r = malloc(sizeof(*r));
	mpz_t *i;
	
	// Loads self into *i
	Data_Get_Struct(self, mpz_t, i);
	
	// Inits the result
	mpz_init(*r);
	
	// (probably) Sets the result as the next prime greater than itself
	mpz_nextprime(*r, *i);
	
	return Data_Wrap_Struct(cGMPInteger, integer_mark, integer_free, r);
}

// Number of digits in a specific base
// {GMP::Integer}, {Fixnum} -> {Fixnum}
VALUE
z_size_in_base( VALUE self, VALUE base ) {
	// TODO: check why 2 has size 2 in base 3
	// GMP's manual mentions that it might be off by one, so I wonder if this
	// method should retain its name...
	
	// Creates a mpz_t pointer and loads self in it
	mpz_t *i;
	Data_Get_Struct(self, mpz_t, i);
	
	// Converts the base from Fixnum to int and checks if its valid
	int intBase = FIX2INT(base);
	if (intBase < 2 || intBase > 62)
		rb_raise(rb_eRangeError, "base out of range");
	
	int size = mpz_sizeinbase(*i, intBase);
	
	return INT2FIX(size);
}

// Efficient swap
// {GMP::Integer}, {GMP::Integer} -> {GMP::Integer}, {GMP::Integer}
VALUE
z_swap( VALUE self, VALUE other ) {
	// Creates pointers to self's and other's mpz_t structures
	mpz_t *i, *o;
	
	// Copies back the mpz_t pointers wrapped in ruby data objects
	Data_Get_Struct(self, mpz_t, i);
	Data_Get_Struct(other, mpz_t, o);
	
	// Swaps the contents of self and other
	mpz_swap(*i, *o);
	
	return Qnil;
}

// Next integer
// {GMP::Integer} -> {GMP::Integer}
VALUE
z_next( VALUE self ) {
	// Creates pointers to self's and the result's mpz_t structures
	mpz_t *r = malloc(sizeof(*r));
	mpz_t *i;
	
	// Loads self into *i
	Data_Get_Struct(self, mpz_t, i);
	
	// Inits the result
	mpz_init(*r);
	
	// Sets the result as the next number from self
	mpz_add_ui(*r, *i, (unsigned long) 1);
	
	return Data_Wrap_Struct(cGMPInteger, integer_mark, integer_free, r);
}

// Gets the specific bit
// {Fixnum, Bignum} -> {Fixnum}
VALUE
z_get_bit( VALUE self, VALUE index ) {
	// Creates a pointer for self's mpz_t structures and two unsigned long
	// for the bit index and desired bit value
	mpz_t *i;
	unsigned long longIndex;
	
	// Loads and checks if the index is within range.
	// Horrible hack: the first NUM2LONG messes up negative indexes
	// while the second one does consider them.
	longIndex = NUM2LONG(index);
	if (NUM2LONG(index) < 0)
		rb_raise(rb_eRangeError, "bit position out of range");
	
	// Copies back the mpz_t pointer wrapped in a ruby data object
	Data_Get_Struct(self, mpz_t, i);
	
	return INT2FIX(mpz_tstbit(*i, longIndex));
}

// Coercion (makes operations commutative)
VALUE
z_coerce( VALUE self, VALUE other ) {
	return rb_assoc_new(
			rb_class_new_instance(1, &other, cGMPInteger),
			self);
}
//// end of other operations
////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////
//// Singletons/Class methods
// Exponetiation with modulo (powermod)
// {GMP::Integer, Fixnum} -> {GMP::Integer}
VALUE
z_powermod( VALUE klass, VALUE self, VALUE exp, VALUE base ) {
	// Creates pointers to self's and the result's mpz_t structures
	mpz_t *r = malloc(sizeof(*r));
	mpz_t *i;
	
	// Loads self into *i
	Data_Get_Struct(self, mpz_t, i);
	
	// Inits the result
	mpz_init(*r);
	
	// Decides what to do based on the exponent's type/class
	// TODO: check if pointer voodoo can let us avoid the switches
	switch (TYPE(exp)) {
		case T_DATA: {
			mpz_t *ed;
			Data_Get_Struct(exp, mpz_t, ed);
			
			switch (TYPE(base)) {
				case T_DATA: {
					mpz_t *bd;
					Data_Get_Struct(base, mpz_t, bd);
					mpz_powm(*r, *i, *ed, *bd);
					break;
				}
				case T_FIXNUM: {
					// This smells like a horrible hack
					// Unfortunately, GMP does not allow the base to be
					// an unsigned long
					unsigned long bl = FIX2LONG(base);
					// Please don't kill me for this
					mpz_t *blTemp = malloc(sizeof(mpz_t));
					mpz_init_set_ui(*blTemp, bl);
					mpz_powm(*r, *i, *ed, *blTemp);
					break;
				}
				default: {
					rb_raise(rb_eTypeError, "base's type not supported");
				}
			}
			
			break;
		}
		case T_FIXNUM: {
			unsigned long el = FIX2LONG(exp);
			
			switch (TYPE(base)) {
				case T_DATA: {
					mpz_t *bd;
					Data_Get_Struct(base, mpz_t, bd);
					mpz_powm_ui(*r, *i, el, *bd);
					break;
				}
				case T_FIXNUM: {
					// This smells like a horrible hack
					// Unfortunately, GMP does not allow the base to be
					// an unsigned long
					unsigned long bl = FIX2LONG(base);
					// Please don't kill me for this
					mpz_t *blTemp = malloc(sizeof(mpz_t));
					mpz_init_set_ui(*blTemp, bl);
					mpz_powm_ui(*r, *i, el, *blTemp);
					break;
				}
				default: {
					rb_raise(rb_eTypeError, "base's type not supported");
				}
			}
			
			break;
		}
		default: {
			rb_raise(rb_eTypeError, "exponent's type not supported");
		}
	}
	
	return Data_Wrap_Struct(cGMPInteger, integer_mark, integer_free, r);
}

// Square root
// {GMP::Integer} -> {GMP::Integer}
VALUE
z_sqrt_singleton( VALUE klass, VALUE number ) {
	// Creates pointers to self's and the result's mpz_t structures
	mpz_t *r = malloc(sizeof(*r));
	mpz_t *n;
	
	// Loads self into *n
	Data_Get_Struct(number, mpz_t, n);
	
	// Inits the result
	mpz_init(*r);
	
	// Checks if the number is positive, raising an exception if not
	if (mpz_sgn(*n) == -1)
		rb_raise(rb_eRuntimeError, "number is negative");
	
	// Takes the floor of the square root
	mpz_sqrt(*r, *n);
	
	return Data_Wrap_Struct(cGMPInteger, integer_mark, integer_free, r);
}

// Nth root
// {GMP::Integer}, {Fixnum} -> {GMP::Integer}
VALUE
z_root_singleton( VALUE klass, VALUE number, VALUE degree ) {
	// Creates pointers to number's and the result's mpz_t structures
	// Also, creates the degree placeholder
	unsigned long longDegree;
	mpz_t *r = malloc(sizeof(*r));
	mpz_t *n;
	
	// Copies back the mpz_t pointer wrapped in a ruby data object
	// as well as the root value
	Data_Get_Struct(number, mpz_t, n);
	longDegree = FIX2LONG(degree);
	
	// If the degree is zero, GMP will normally give a floating point error
	// A possible solution is to make the degree -1, which works as expected
	if (longDegree == (unsigned long) 0)
		--longDegree;
	
	// Raise an exception if both the number is negative and the
	// degree is even
	if (mpz_sgn(*n) == -1 && longDegree % 2 == 0)
		rb_raise(rb_eRuntimeError, "number is negative, but degree is even");
	
	// Inits the result
	mpz_init(*r);
	
	mpz_root(*r, *n, longDegree);
	
	return Data_Wrap_Struct(cGMPInteger, integer_mark, integer_free, r);
}

// Fibonacci numbers generator
// {Fixnum} -> {GMP::Integer}
VALUE
z_fibonacci_singleton( VALUE klass, VALUE index ) {
	// Creates pointer to the result's mpz_t structure, and loads
	// creates a placeholder for the index
	mpz_t *r = malloc(sizeof(*r));
	unsigned long longIndex;
	
	// Loads the index into a long
	longIndex = FIX2LONG(index);
	
	// Inits the result
	mpz_init(*r);
	
	mpz_fib_ui(*r, longIndex);
	
	return Data_Wrap_Struct(cGMPInteger, integer_mark, integer_free, r);
}

// Fibonacci pairs generator
// {Fixnum} -> {Array <GMP::Integer> (2)}
VALUE
z_fibonacci2_singleton( VALUE klass, VALUE index ) {
	// Creates placeholders for the two results
	mpz_t *x = malloc(sizeof(*x));
	mpz_t *y = malloc(sizeof(*y));
	
	// Loads the index into a long
	unsigned long longIndex = FIX2LONG(index);
	
	// Inits both results
	mpz_init(*x);
	mpz_init(*y);
	
	// Does the calculation
	mpz_fib2_ui(*x, *y, longIndex);
	
	// Converts the results into Ruby data objects
	VALUE rx = Data_Wrap_Struct(cGMPInteger, integer_mark, integer_free, x);
	VALUE ry = Data_Wrap_Struct(cGMPInteger, integer_mark, integer_free, y);
	
	return rb_ary_new3(2, ry, rx);
}

// Lucas numbers generator
// {Fixnum} -> {GMP::Integer}
VALUE
z_lucas_singleton( VALUE klass, VALUE index ) {
	// Creates pointer to the result's mpz_t structure, and loads
	// creates a placeholder for the index
	mpz_t *r = malloc(sizeof(*r));
	unsigned long longIndex;
	
	// Loads the index into a long
	longIndex = FIX2LONG(index);
	
	// Inits the result
	mpz_init(*r);
	
	mpz_lucnum_ui(*r, longIndex);
	
	return Data_Wrap_Struct(cGMPInteger, integer_mark, integer_free, r);
}

// Lucas pairs generator
// {Fixnum} -> {Array <GMP::Integer> (2)}
VALUE
z_lucas2_singleton( VALUE klass, VALUE index ) {
	// Creates placeholders for the two results
	mpz_t *x = malloc(sizeof(*x));
	mpz_t *y = malloc(sizeof(*y));
	
	// Loads the index into a long
	unsigned long longIndex = FIX2LONG(index);
	
	// Inits both results
	mpz_init(*x);
	mpz_init(*y);
	
	// Does the calculation
	mpz_lucnum2_ui(*x, *y, longIndex);
	
	// Converts the results into Ruby data objects
	VALUE rx = Data_Wrap_Struct(cGMPInteger, integer_mark, integer_free, x);
	VALUE ry = Data_Wrap_Struct(cGMPInteger, integer_mark, integer_free, y);
	
	return rb_ary_new3(2, ry, rx);
}

// Factorial (n!)
// {Fixnum} -> {GMP::Integer}
VALUE
z_factorial_singleton( VALUE klass, VALUE number ) {
	// Creates pointer to the result's mpz_t structure, and loads
	// creates a placeholder for the number
	mpz_t *r = malloc(sizeof(*r));
	unsigned long longNumber;
	
	// Loads the number into a long
	longNumber = FIX2LONG(number);
	
	// Inits the result
	mpz_init(*r);
	
	mpz_fac_ui(*r, longNumber);
	
	return Data_Wrap_Struct(cGMPInteger, integer_mark, integer_free, r);
}

// Binomial coefficient/Combination (combinatorics)
// {GMP::Integer}, {Fixnum} -> {GMP::Integer}
// TODO: check whether N and K have names
VALUE
z_binomial_singleton( VALUE klass, VALUE n, VALUE k ) {
	// Checks whether k has a valid type
	if (TYPE(k) != T_FIXNUM)
		rb_raise(rb_eTypeError, "k is not of a supported data type");
	
	// Creates pointer to the result's mpz_t structure, and loads
	// creates a placeholder for the number
	mpz_t *r = malloc(sizeof(*r));
	unsigned long longK;
	
	// Loads the number into a long
	longK = FIX2LONG(k);
	
	// Inits the result
	mpz_init(*r);
	
	// Decides what to do based on n's type
	switch (TYPE(n)) {
		case T_DATA: {
			mpz_t *sd;
			Data_Get_Struct(n, mpz_t, sd);
			mpz_bin_ui(*r, *sd, longK);
			break;
		}
		case T_FIXNUM: {
			unsigned long sl = FIX2LONG(n);
			mpz_bin_uiui(*r, sl, longK);
			break;
		}
		default: {
			rb_raise(rb_eTypeError, "n is not of a supported data type");
		}
	}
	
	return Data_Wrap_Struct(cGMPInteger, integer_mark, integer_free, r);
}

// Factor removal (divides exhaustively by one factor until no longer possible)
// {GMP::Integer}, {GMP::Integer} -> {GMP::Integer}
// TODO: decide how (and if) to handle mpz_remove's output
VALUE
z_remove_singleton( VALUE klass, VALUE number, VALUE factor ) {
	// Creates pointers to the number's, factor's and result's mpz_t
	// structures
	mpz_t *r = malloc(sizeof(*r));
	mpz_t *n, *f;
	
	// Copies back the mpz_t pointers wrapped in ruby data objects
	Data_Get_Struct(number, mpz_t, n);
	Data_Get_Struct(factor, mpz_t, f);
	
	if (mpz_cmp_ui(*f, 1) <= 0)
		rb_raise(rb_eRangeError, "factor must be bigger than 1");
	
	// Inits the result
	mpz_init(*r);
	
	mpz_remove(*r, *n, *f);
	
	return Data_Wrap_Struct(cGMPInteger, integer_mark, integer_free, r);
}

// Comparison of absolutes
// {GMP::Integer}, {GMP::Integer, Fixnum} -> {Fixnum}
VALUE
z_comp_abs_singleton( VALUE klass, VALUE number, VALUE other ) {
	// Creates pointers to the number's mpz_t structures
	// Also creates the int placeholder for the result
	// TODO: check whether all these declarations without immediate
	// attributions give a performance penalty
	mpz_t *n;
	int result;
	
	// Copies back the mpz_t pointer wrapped in a ruby data object
	Data_Get_Struct(number, mpz_t, n);
	
	switch (TYPE(other)) {
		case T_DATA: {
			if (rb_obj_class(other) == cGMPInteger) {
				// Creates and loads the mpz_t structure for other
				mpz_t *od;
				Data_Get_Struct(other, mpz_t, od);
				result = mpz_cmpabs(*n, *od);
			} else {
				rb_raise(rb_eTypeError, "input data type not supported");
			}
			break;
		}
		case T_FIXNUM: {
			// Loads other into an unsigned long
			unsigned long ol = FIX2LONG(other);
			result = mpz_cmpabs_ui(*n, ol);
			break;
		}
		default: {
			rb_raise(rb_eTypeError, "input data type not supported");
		}
	}
	
	return INT2FIX(result);
}

// Inversion (number theory; a*ã == 1 (mod m))
// {GMP::Integer} -> {GMP::Integer}
VALUE
z_invert_singleton( VALUE klass, VALUE number, VALUE base ) {
	// Creates pointers to the number's, base's and result's mpz_t
	// structures
	// Also creates an int which will be used to check if the number has an
	// inverse on that base
	mpz_t *r = malloc(sizeof(*r));
	mpz_t *n, *b;
	int check;
	
	// Copies back the mpz_t pointers wrapped in ruby data objects
	Data_Get_Struct(number, mpz_t, n);
	Data_Get_Struct(base, mpz_t, b);
	
	// Inits the result
	mpz_init(*r);
	
	check = mpz_invert(*r, *n, *b);
	
	if (check == 0)
		rb_raise(rb_eRuntimeError, "input is not invertible on this base");
	
	return Data_Wrap_Struct(cGMPInteger, integer_mark, integer_free, r);
}

// Least common multiple
// {GMP::Integer}, {GMP::Integer, Fixnum} -> {GMP::Integer}
VALUE
z_lcm_singleton( VALUE klass, VALUE number, VALUE other ) {
	// Creates pointers to self's and the result's mpz_t structures
	mpz_t *r = malloc(sizeof(*r));
	mpz_t *n;
	
	// Loads self into *n
	Data_Get_Struct(number, mpz_t, n);
	
	// Inits the result
	mpz_init(*r);
	
	// Decides what to do based on other's type/class
	switch (TYPE(other)) {
		case T_DATA: {
			if (rb_obj_class(other) == cGMPInteger) {
				mpz_t *od;
				Data_Get_Struct(other, mpz_t, od);
				mpz_lcm(*r, *n, *od);
			} else {
				rb_raise(rb_eTypeError, "input data type not supported");
			}
			break;
		}
		case T_FIXNUM: {
			unsigned long ol = FIX2LONG(other);
			mpz_lcm_ui(*r, *n, ol);
			break;
		}
		default: {
			rb_raise(rb_eTypeError, "input data type not supported");
		}
	}
	
	return Data_Wrap_Struct(cGMPInteger, integer_mark, integer_free, r);
}

// Greatest common divisor
// {GMP::Integer}, {GMP::Integer} -> {GMP::Integer}
VALUE
z_gcd_singleton( VALUE klass, VALUE number, VALUE other ) {
	// Creates pointers to self's and the result's mpz_t structures
	mpz_t *r = malloc(sizeof(*r));
	mpz_t *n;
	
	// Loads self into *n
	Data_Get_Struct(number, mpz_t, n);
	
	// Inits the result
	mpz_init(*r);
	
	// Decides what to do based on other's type/class
	switch (TYPE(other)) {
		case T_DATA: {
			if (rb_obj_class(other) == cGMPInteger) {
				mpz_t *od;
				Data_Get_Struct(other, mpz_t, od);
				mpz_gcd(*r, *n, *od);
			} else {
				rb_raise(rb_eTypeError, "input data type not supported");
			}
			break;
		}
		case T_FIXNUM: {
			unsigned long ol = FIX2LONG(other);
			mpz_gcd_ui(*r, *n, ol);
			break;
		}
		default: {
			rb_raise(rb_eTypeError, "input data type not supported");
		}
	}
	
	return Data_Wrap_Struct(cGMPInteger, integer_mark, integer_free, r);
}

// Jacobi symbol
// {GMP::Integer}, {GMP::Integer} -> {Fixnum}
VALUE
z_jacobi_singleton( VALUE klass, VALUE a, VALUE b ) {
	// Creates pointers to a's and b's mpz_t structures
	// Also creates the int placeholder for the result
	mpz_t *az, *bz;
	int result;
	
	// Copies back the mpz_t pointers wrapped in ruby data objects
	Data_Get_Struct(a, mpz_t, az);
	Data_Get_Struct(b, mpz_t, bz);
	
	// Not sure if this is necessary, and it probably adds a little overhead
	// GMP does not complain if the following is not the case
	// Checks if b is odd
	if (! mpz_odd_p(*bz))
		rb_raise(rb_eRuntimeError, "b is not odd");
	
	result = mpz_jacobi(*az, *bz);
	
	return INT2FIX(result);
}

// Kronecker symbol
// {GMP::Integer}, {GMP::Integer} -> {Fixnum}
VALUE
z_kronecker( VALUE klass, VALUE a, VALUE b ) {
	// Creates pointers to a's and b's mpz_t structures
	// Also creates the int placeholder for the result
	mpz_t *az, *bz;
	int result;
	
	// Copies back the mpz_t pointers wrapped in ruby data objects
	Data_Get_Struct(a, mpz_t, az);
	Data_Get_Struct(b, mpz_t, bz);
	
	result = mpz_kronecker(*az, *bz);
	
	return INT2FIX(result);
}

// Greatest Common Divisor - gcd and pair of coefficients (gcd(a,b) = a*s + b*t)
// {GMP::Integer}, {GMP::Integer} -> {Array <GMP::Integer> (3))
VALUE
z_extended_gcd( VALUE klass, VALUE a, VALUE b ) {
	// Creates pointers to a's, b's, and the results' structures.
	mpz_t *g = malloc(sizeof(*g));
	mpz_t *s = malloc(sizeof(*s));
	mpz_t *t = malloc(sizeof(*t));
	mpz_t *ma, *mb;
	
	// Loads a and b
	Data_Get_Struct(a, mpz_t, ma);
	Data_Get_Struct(b, mpz_t, mb);
	
	// Inits the results
	mpz_init(*t);
	mpz_init(*s);
	mpz_init(*g);
	
	// Does the calculation
	mpz_gcdext(*g, *s, *t, *ma, *mb);
	
	// Wraps ther esults into Ruby objects
	VALUE rt = Data_Wrap_Struct(cGMPInteger, integer_mark, integer_free, t);
	VALUE rs = Data_Wrap_Struct(cGMPInteger, integer_mark, integer_free, s);
	VALUE rg = Data_Wrap_Struct(cGMPInteger, integer_mark, integer_free, g);
	
	return rb_ary_new3(3, rg, rs, rt);
}
//// end of singleton/class methods
////////////////////////////////////////////////////////////////////



void
Init_gmpz() {
	// Defines the module GMP and class GMP::Integer
	mGMP = rb_define_module("GMP");
	cGMPInteger = rb_define_class_under(mGMP, "Integer", rb_cObject);
	
	// Book keeping and the constructor method
	rb_define_alloc_func(cGMPInteger, integer_allocate);
	rb_define_method(cGMPInteger, "initialize", z_init, 1);
	
	// Converters
	rb_define_method(cGMPInteger, "to_s", z_to_string, -1);
	rb_define_method(cGMPInteger, "to_i", z_to_integer, 0);
	rb_define_method(cGMPInteger, "to_f", z_to_float, 0);
	
	// Binary operators
	rb_define_method(cGMPInteger, "+", z_addition, 1);
	rb_define_method(cGMPInteger, "-", z_subtraction, 1);
	rb_define_method(cGMPInteger, "*", z_multiplication, 1);
	rb_define_method(cGMPInteger, "/", z_division, 1);
	rb_define_method(cGMPInteger, "%", z_modulo, 1);
	rb_define_method(cGMPInteger, "**", z_power, 1);
	rb_define_method(cGMPInteger, "<<", z_left_shift, 1);
	rb_define_method(cGMPInteger, ">>", z_right_shift, 1);
	
	// Unary operators
	rb_define_method(cGMPInteger, "+@", z_positive, 0);
	rb_define_method(cGMPInteger, "-@", z_negation, 0);
	
	// Logic operators
	rb_define_method(cGMPInteger, "&", z_logic_and, 1);
	rb_define_method(cGMPInteger, "|", z_logic_ior, 1);
	rb_define_method(cGMPInteger, "^", z_logic_xor, 1);
	rb_define_method(cGMPInteger, "com", z_logic_not, 0);
	
	// Comparisons
	rb_define_method(cGMPInteger, "==", z_equality_test, 1);
	rb_define_method(cGMPInteger, ">", z_greater_than_test, 1);
	rb_define_method(cGMPInteger, "<", z_less_than_test, 1);
	rb_define_method(cGMPInteger, ">=", z_greater_than_or_equal_to_test, 1);
	rb_define_method(cGMPInteger, "<=", z_less_than_or_equal_to_test, 1);
	rb_define_method(cGMPInteger, "<=>", z_generic_comparison, 1);
	
	// Inplace methods
	rb_define_method(cGMPInteger, "abs!", z_absolute_inplace, 0);
	rb_define_method(cGMPInteger, "neg!", z_negation_inplace, 0);
	rb_define_method(cGMPInteger, "next_prime!", z_next_prime_inplace, 0);
	rb_define_method(cGMPInteger, "sqrt!", z_sqrt_inplace, 0);
	rb_define_method(cGMPInteger, "root!", z_root_inplace, 1);
	rb_define_method(cGMPInteger, "invert!", z_invert_inplace, 1);
	rb_define_method(cGMPInteger, "[]=", z_set_bit_inplace, 2);
	rb_define_method(cGMPInteger, "add!", z_addition_inplace, 1);
	rb_define_method(cGMPInteger, "sub!", z_subtraction_inplace, 1);
	rb_define_method(cGMPInteger, "mul!", z_multiplication_inplace, 1);
	rb_define_method(cGMPInteger, "addmul", z_addmul_inplace, 2);
	rb_define_method(cGMPInteger, "submul", z_submul_inplace, 2);
	
	// Question-like methods
	rb_define_method(cGMPInteger, "divisible_by?", z_divisible, 1);
	rb_define_method(cGMPInteger, "perfect_power?", z_perfect_power, 0);
	rb_define_method(cGMPInteger, "perfect_square?", z_perfect_square, 0);
	rb_define_method(cGMPInteger, "probable_prime?", z_probable_prime, 1);
	rb_define_method(cGMPInteger, "even?", z_even, 0);
	rb_define_method(cGMPInteger, "odd?", z_odd, 0);
	rb_define_method(cGMPInteger, "eql?", z_precise_equality, 1);
	rb_define_method(cGMPInteger, "zero?", z_zero, 0);
	rb_define_method(cGMPInteger, "nonzero?", z_nonzero, 0);
	
	// Other operations
	rb_define_method(cGMPInteger, "abs", z_absolute, 0);
	rb_define_method(cGMPInteger, "next_prime", z_next_prime, 0);
	rb_define_method(cGMPInteger, "size_in_base", z_size_in_base, 1);
	rb_define_method(cGMPInteger, "swap", z_swap, 1);
	rb_define_method(cGMPInteger, "next", z_next, 0);
	rb_define_method(cGMPInteger, "[]", z_get_bit, 1);
	rb_define_method(cGMPInteger, "coerce", z_coerce, 1);
	
	// Singletons/Class methods
	rb_define_singleton_method(cGMPInteger, "powermod", z_powermod, 3);
	rb_define_singleton_method(cGMPInteger, "sqrt", z_sqrt_singleton, 1);
	rb_define_singleton_method(cGMPInteger, "root", z_root_singleton, 2);
	rb_define_singleton_method(cGMPInteger, "fib", z_fibonacci_singleton, 1);
	rb_define_singleton_method(cGMPInteger, "fib2", z_fibonacci2_singleton, 1);
	rb_define_singleton_method(cGMPInteger, "luc", z_lucas_singleton, 1);
	rb_define_singleton_method(cGMPInteger, "luc2", z_lucas2_singleton, 1);
	rb_define_singleton_method(cGMPInteger, "fac", z_factorial_singleton, 1);
	rb_define_singleton_method(cGMPInteger, "bin", z_binomial_singleton, 2);
	rb_define_singleton_method(cGMPInteger, "remove", z_remove_singleton, 2);
	rb_define_singleton_method(cGMPInteger, "cmpabs", z_comp_abs_singleton, 2);
	rb_define_singleton_method(cGMPInteger, "invert", z_invert_singleton, 2);
	rb_define_singleton_method(cGMPInteger, "lcm", z_lcm_singleton, 2);
	rb_define_singleton_method(cGMPInteger, "gcd", z_gcd_singleton, 2);
	rb_define_singleton_method(cGMPInteger, "jacobi", z_jacobi_singleton, 2);
	rb_define_singleton_method(cGMPInteger, "kronecker", z_kronecker, 2);
	rb_define_singleton_method(cGMPInteger, "xgcd", z_extended_gcd, 2);
	
	// Aliases
	rb_define_alias(cGMPInteger, "modulo", "%");
	rb_define_alias(cGMPInteger, "magnitude", "abs");
	rb_define_alias(cGMPInteger, "to_int", "to_i");
	rb_define_alias(cGMPInteger, "to_d", "to_f");
	rb_define_alias(cGMPInteger, "not", "com");
	// Whether or not this is a good idea is debatable, but for now...
	rb_define_singleton_method(cGMPInteger, "legendre", z_jacobi_singleton, 2);
}
