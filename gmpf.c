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

static VALUE mGMP;
static VALUE cGMPFloat;

////////////////////////////////////////////////////////////////////
//// Fundamental methods
// Garbage collection
static void
float_mark( mpf_t *f ) {}

static void
float_free( mpf_t *f ) {
	mpf_clear(*f);
}

// Object allocation
static VALUE
float_allocate( VALUE klass ) {
	mpf_t *f = malloc(sizeof(mpf_t));
	mpf_init(*f);
	return Data_Wrap_Struct(klass, float_mark, float_free, f);
}

// Class constructor
// Precision must be a Fixnum
static VALUE
f_init( VALUE argc, VALUE *argv, VALUE self ) {
	// Creates a mpf_t pointer for the new object
	mpf_t *s;
	
	// Creates placeholders for the arguments
	VALUE number, precision;
	
	// The first argument is mandatory (the value this object will assign).
	// The second argument is optional (precision to be used).
	rb_scan_args(argc, argv, "11", &number, &precision);
	
	// Loads the (blank) new object
	Data_Get_Struct(self, mpf_t, s);
	
	switch (TYPE(number)) {
		case T_STRING: {
			mpf_set_str(*s, StringValuePtr(number), 10);
			break;
		}
		case T_FLOAT: {
			double nf = RFLOAT_VALUE(number);
			mpf_set_d(*s, nf);
			break;
		}
		default: {
			rb_raise(rb_eTypeError, "input data type not supported");
		}
	}
	
	if (precision != Qnil)
		mpf_set_prec(*s, FIX2LONG(precision));
	
	return Qnil;
}
//// end of fundamental methods
////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////
//// Conversion methods (from C types to Ruby classes)
// To String
// Not yet working (we must place the dot in there somewhere)
static VALUE
f_to_string( VALUE self ) {
	// Creates a mpf_t pointer and loads self into it
	mpf_t *s;
	Data_Get_Struct(self, mpf_t, s);
	
	// Creates the pointer to the string and loads it from GMP
	mp_exp_t exp;
	char *str = mpf_get_str(NULL, &exp, 10, 0, *s);
	
	VALUE string = rb_str_new2(str);
	free(str);
	
	return string;
}

// To Float
// {GMP::Float} -> {Float}
static VALUE
f_to_float( VALUE self ) {
	// Creates a mpf_t pointer and loads self into it
	mpf_t *s;
	Data_Get_Struct(self, mpf_t, s);
	
	// Converts mpf_t to C double, truncating if required (done internally)
	double result = mpf_get_d(*s);
	
	return rb_float_new(result);
}
//// end of conversion methods
////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////
//// Binary arithmetical operators
// Addition (+)
// {GMP::Float, Float, Fixnum, Bignum} -> {GMP::Float}
static VALUE
f_addition( VALUE self, VALUE summand ) {
	// Creates pointers to self's and the result's mpf_t structures
	mpf_t *r = malloc(sizeof(*r));
	mpf_t *f;
	
	// Loads self into *f
	Data_Get_Struct(self, mpf_t, f);
	
	// Inits the result
	mpf_init(*r);
	
	// Decides what to do based on the summand's type/class
	switch (TYPE(summand)) {
		case T_DATA: {
			if (rb_obj_class(summand) == cGMPFloat) {
				mpf_t *sd;
				Data_Get_Struct(summand, mpf_t, sd);
				mpf_add(*r, *f, *sd);
			} else {
				rb_raise(rb_eTypeError, "input data type not supported");
			}
			break;
		}
		case T_FLOAT: {
			// GMP does not provide support for adding a mpf_t to a C double
			// so first we have to convert Ruby's Float to a mpf_t and then do
			// the operation
			mpf_t tempSufl;
			mpf_init_set_d(tempSufl, NUM2DBL(summand));
			mpf_add(*r, *f, tempSufl);
			mpf_clear(tempSufl);
			break;
		}
		case T_FIXNUM: {
			// Likewise, Ruby's Fixnum should first be converted to a mpf_float
			// to keep the sign.
			// I have a feeling this can be done otherwise, with some size
			// checking, though it would still feel like a bad hack.
			mpf_t tempSufi;
			mpf_init_set_si(tempSufi, FIX2LONG(summand));
			mpf_add(*r, *f, tempSufi);
			mpf_clear(tempSufi);
			break;
		}
		case T_BIGNUM: {
			mpf_t tempSub;
			VALUE str = rb_big2str(summand, 10);
			mpf_init_set_str(tempSub, StringValuePtr(str), 10);
			mpf_add(*r, *f, tempSub);
			mpf_clear(tempSub);
			break;
		}
		default: {
			rb_raise(rb_eTypeError, "input data type not supported");
		}
	}
	
	return Data_Wrap_Struct(cGMPFloat, float_mark, float_free, r);
}

// Subtraction (-)
// {GMP::Float, Float, Fixnum, Bignum} -> {GMP::Float}
static VALUE
f_subtraction( VALUE self, VALUE subtraend ) {
	// Creates pointers to self's and the result's mpf_t structures
	mpf_t *r = malloc(sizeof(*r));
	mpf_t *f;
	
	// Loads self into *f
	Data_Get_Struct(self, mpf_t, f);
	
	// Inits the result
	mpf_init(*r);
	
	// Decides what to do based on the subtraend's type/class
	switch (TYPE(subtraend)) {
		case T_DATA: {
			if (rb_obj_class(subtraend) == cGMPFloat) {
				mpf_t *sd;
				Data_Get_Struct(subtraend, mpf_t, sd);
				mpf_sub(*r, *f, *sd);
			} else {
				rb_raise(rb_eTypeError, "input data type not supported");
			}
			break;
		}
		case T_FLOAT: {
			// GMP does not provide support for adding a mpf_t to a C double
			// so first we have to convert Ruby's Float to a mpf_t and then do
			// the operation
			mpf_t tempSufl;
			mpf_init_set_d(tempSufl, NUM2DBL(subtraend));
			mpf_sub(*r, *f, tempSufl);
			mpf_clear(tempSufl);
			break;
		}
		case T_FIXNUM: {
			// Likewise, Ruby's Fixnum should first be converted to a mpf_float
			// to keep the sign.
			// I have a feeling this can be done otherwise, with some size
			// checking, though it would still feel like a bad hack.
			mpf_t tempSufi;
			mpf_init_set_si(tempSufi, FIX2LONG(subtraend));
			mpf_sub(*r, *f, tempSufi);
			mpf_clear(tempSufi);
			break;
		}
		case T_BIGNUM: {
			mpf_t tempSub;
			VALUE str = rb_big2str(subtraend, 10);
			mpf_init_set_str(tempSub, StringValuePtr(str), 10);
			mpf_sub(*r, *f, tempSub);
			mpf_clear(tempSub);
			break;
		}
		default: {
			rb_raise(rb_eTypeError, "input data type not supported");
		}
	}
	
	return Data_Wrap_Struct(cGMPFloat, float_mark, float_free, r);
}

// Multiplication (*)
// {GMP::Float, Float, Fixnum, Bignum} -> {GMP::Float}
static VALUE
f_multiplication( VALUE self, VALUE multiplicand ) {
	// Creates pointers to self's and the result's mpf_t structures
	mpf_t *r = malloc(sizeof(*r));
	mpf_t *f;
	
	// Loads self into *f
	Data_Get_Struct(self, mpf_t, f);
	
	// Inits the result
	mpf_init(*r);
	
	// Decides what to do based on the multiplicand's type/class
	switch (TYPE(multiplicand)) {
		case T_DATA: {
			if (rb_obj_class(multiplicand) == cGMPFloat) {
				mpf_t *md;
				Data_Get_Struct(multiplicand, mpf_t, md);
				mpf_mul(*r, *f, *md);
			} else {
				rb_raise(rb_eTypeError, "input data type not supported");
			}
			break;
		}
		case T_FLOAT: {
			// GMP does not provide support for multiplying a mpf_t with a
			// C double, so first we have to convert Ruby's Float to a mpf_t
			// and then do the operation
			mpf_t tempMufl;
			mpf_init_set_d(tempMufl, NUM2DBL(multiplicand));
			mpf_mul(*r, *f, tempMufl);
			mpf_clear(tempMufl);
			break;
		}
		case T_FIXNUM: {
			signed long ml = FIX2LONG(multiplicand);
			mpf_mul_ui(*r, *f, (ml > 0) ? ml : -ml);
			if (ml < 0)
				mpf_neg(*r, *r);
			break;
		}
		case T_BIGNUM: {
			mpf_t tempMub;
			VALUE str = rb_big2str(multiplicand, 10);
			mpf_init_set_str(tempMub, StringValuePtr(str), 10);
			mpf_mul(*r, *f, tempMub);
			mpf_clear(tempMub);
			break;
		}
		default: {
			rb_raise(rb_eTypeError, "input data type not supported");
		}
	}
	
	return Data_Wrap_Struct(cGMPFloat, float_mark, float_free, r);
}

// Division (/)
// {GMP::Float, Float, Fixnum, Bignum} -> {GMP::Float}
static VALUE
f_division( VALUE self, VALUE dividend ) {
	// Creates pointers to self's and the result's mpf_t structures
	mpf_t *r = malloc(sizeof(*r));
	mpf_t *f;
	
	// Loads self into *f
	Data_Get_Struct(self, mpf_t, f);
	
	// Inits the result
	mpf_init(*r);
	
	// Decides what to do based on the dividend's type/class
	switch (TYPE(dividend)) {
		case T_DATA: {
			if (rb_obj_class(dividend) == cGMPFloat) {
				mpf_t *dd;
				Data_Get_Struct(dividend, mpf_t, dd);
				mpf_div(*r, *f, *dd);
			} else {
				rb_raise(rb_eTypeError, "dividend's class not supported");
			}
			break;
		}
		case T_FLOAT: {
			// GMP does not provide support for dividing a mpf_t by a
			// C double, so first we have to convert Ruby's Float to a mpf_t
			// and then do the operation
			mpf_t tempDifl;
			mpf_init_set_d(tempDifl, NUM2DBL(dividend));
			mpf_div(*r, *f, tempDifl);
			mpf_clear(tempDifl);
			break;
		}
		case T_FIXNUM: {
			signed long dl = FIX2LONG(dividend);
			mpf_div_ui(*r, *f, (dl > 0) ? dl : -dl);
			if (dl < 0)
				mpf_neg(*r, *r);
			break;
		}
		case T_BIGNUM: {
			mpf_t tempDib;
			VALUE str = rb_big2str(dividend, 10);
			mpf_init_set_str(tempDib, StringValuePtr(str), 10);
			mpf_div(*r, *f, tempDib);
			mpf_clear(tempDib);
			break;
		}
		default: {
			rb_raise(rb_eTypeError, "dividend's class not supported");
		}
	}
	
	return Data_Wrap_Struct(cGMPFloat, float_mark, float_free, r);
}

// Exponentiation (**)
// {Fixnum} -> {GMP::Float}
// TODO: incorporate MPFR
static VALUE
f_power( VALUE self, VALUE exponent ) {
	// Creates pointers to self's and the result's mpf_t structures
	mpf_t *r = malloc(sizeof(*r));
	mpf_t *f;
	
	// Loads self into *f
	Data_Get_Struct(self, mpf_t, f);
	
	// Inits the result
	mpf_init(*r);
	
	// GMP only supports positive integral exponents
	if (!FIXNUM_P(exponent))
		rb_raise(rb_eTypeError, "exponent must be a (positive) Fixnum");
	mpf_pow_ui(*r, *f, FIX2LONG(exponent));
	
	return Data_Wrap_Struct(cGMPFloat, float_mark, float_free, r);
}
//// end of binary operator methods
////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////
//// Unary arithmetical operators (operations over a single value)
// Plus (+a)
// {GMP::Float} -> {GMP::Float}
static VALUE
f_positive( VALUE self ) {
	return self;
}

// Negation (-a)
// {GMP::Float} -> {GMP::Float}
static VALUE
f_negation( VALUE self ) {
	// Creates pointers to self's and the result's mpf_t structures
	mpf_t *r = malloc(sizeof(*r));
	mpf_t *f;
	
	// Loads self into *f
	Data_Get_Struct(self, mpf_t, f);
	
	// Inits the result
	mpf_init(*r);
	
	// Negates i, and copies the result to r
	mpf_neg(*r, *f);
	
	return Data_Wrap_Struct(cGMPFloat, float_mark, float_free, r);
}
//// end of unary operator methods
////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////
//// Comparison methods
// Equality (==)
// {GMP::Float, Float} -> {TrueClass, FalseClass}
static VALUE
f_equality_test( VALUE self, VALUE other ) {
	// Creates a mpf_t pointer and loads self in it
	mpf_t *f;
	Data_Get_Struct(self, mpf_t, f);
	
	// Immediate responses to avoid one object creation
	switch (TYPE(other)) {
		case T_DATA: {
			if (rb_obj_class(other) == cGMPFloat) {
				mpf_t *od;
				Data_Get_Struct(other, mpf_t, od);
				
				if (mpf_cmp(*f, *od) == 0)
					return Qtrue;
				else
					return Qfalse;
			} else {
				// I seriously need to start working on these error messages
				rb_raise(rb_eTypeError, "input data type not supported");
			}
			break;
		}
		case T_FLOAT: {
			double of = RFLOAT_VALUE(other);
			
			if (mpf_cmp_d(*f, of) == 0)
				return Qtrue;
			else
				return Qfalse;
			
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
// {GMP::Float, Float} -> {TrueClass, FalseClass}
static VALUE
f_greater_than_test( VALUE self, VALUE other ) {
	// Creates a mpf_t pointer and loads self in it
	mpf_t *f;
	Data_Get_Struct(self, mpf_t, f);
	
	// Immediate responses to avoid one object creation
	switch (TYPE(other)) {
		case T_DATA: {
			if (rb_obj_class(other) == cGMPFloat) {
				mpf_t *od;
				Data_Get_Struct(other, mpf_t, od);
				
				if (mpf_cmp(*f, *od) > 0)
					return Qtrue;
				else
					return Qfalse;
			} else {
				// I seriously need to start working on these error messages
				rb_raise(rb_eTypeError, "input data type not supported");
			}
			break;
		}
		case T_FLOAT: {
			double of = RFLOAT_VALUE(other);
			
			if (mpf_cmp_d(*f, of) > 0)
				return Qtrue;
			else
				return Qfalse;
			
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

// Less than (>)
// {GMP::Float, Float} -> {TrueClass, FalseClass}
static VALUE
f_less_than_test( VALUE self, VALUE other ) {
	// Creates a mpf_t pointer and loads self in it
	mpf_t *f;
	Data_Get_Struct(self, mpf_t, f);
	
	// Immediate responses to avoid one object creation
	switch (TYPE(other)) {
		case T_DATA: {
			if (rb_obj_class(other) == cGMPFloat) {
				mpf_t *od;
				Data_Get_Struct(other, mpf_t, od);
				
				if (mpf_cmp(*f, *od) < 0)
					return Qtrue;
				else
					return Qfalse;
			} else {
				// I seriously need to start working on these error messages
				rb_raise(rb_eTypeError, "input data type not supported");
			}
			break;
		}
		case T_FLOAT: {
			double of = RFLOAT_VALUE(other);
			
			if (mpf_cmp_d(*f, of) < 0)
				return Qtrue;
			else
				return Qfalse;
			
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
// {GMP::Float, Float} -> {TrueClass, FalseClass}
static VALUE
f_greater_than_or_equal_to_test( VALUE self, VALUE other ) {
	// Creates a mpf_t pointer and loads self in it
	mpf_t *f;
	Data_Get_Struct(self, mpf_t, f);
	
	// Immediate responses to avoid one object creation
	switch (TYPE(other)) {
		case T_DATA: {
			if (rb_obj_class(other) == cGMPFloat) {
				mpf_t *od;
				Data_Get_Struct(other, mpf_t, od);
				
				if (mpf_cmp(*f, *od) >= 0)
					return Qtrue;
				else
					return Qfalse;
			} else {
				// I seriously need to start working on these error messages
				rb_raise(rb_eTypeError, "input data type not supported");
			}
			break;
		}
		case T_FLOAT: {
			double of = RFLOAT_VALUE(other);
			
			if (mpf_cmp_d(*f, of) >= 0)
				return Qtrue;
			else
				return Qfalse;
			
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
// {GMP::Float, Float} -> {TrueClass, FalseClass}
static VALUE
f_less_than_or_equal_to_test( VALUE self, VALUE other ) {
	// Creates a mpf_t pointer and loads self in it
	mpf_t *f;
	Data_Get_Struct(self, mpf_t, f);
	
	// Immediate responses to avoid one object creation
	switch (TYPE(other)) {
		case T_DATA: {
			if (rb_obj_class(other) == cGMPFloat) {
				mpf_t *od;
				Data_Get_Struct(other, mpf_t, od);
				
				if (mpf_cmp(*f, *od) <= 0)
					return Qtrue;
				else
					return Qfalse;
			} else {
				// I seriously need to start working on these error messages
				rb_raise(rb_eTypeError, "input data type not supported");
			}
			break;
		}
		case T_FLOAT: {
			double of = RFLOAT_VALUE(other);
			
			if (mpf_cmp_d(*f, of) <= 0)
				return Qtrue;
			else
				return Qfalse;
			
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
// {GMP::Float, Float} -> {Fixnum}
static VALUE
f_generic_comparison( VALUE self, VALUE other ) {
	// Creates a mpf_t pointer and loads self in it
	mpf_t *f;
	Data_Get_Struct(self, mpf_t, f);
	
	// Immediate responses to avoid one object creation
	switch (TYPE(other)) {
		case T_DATA: {
			if (rb_obj_class(other) == cGMPFloat) {
				mpf_t *od;
				Data_Get_Struct(other, mpf_t, od);
				
				return INT2FIX(mpf_cmp(*f, *od));
			} else {
				// I seriously need to start working on these error messages
				rb_raise(rb_eTypeError, "input data type not supported");
			}
			break;
		}
		case T_FLOAT: {
			double of = RFLOAT_VALUE(other);
			
			return INT2FIX(mpf_cmp_d(*f, of));
			
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
//// Question-like methods
// Is it an integer?
// {} -> {TrueClass, FalseClass)
static VALUE
f_integer( VALUE self ) {
	// Creates a mpf_t pointer and loads self into it
	mpf_t *s;
	Data_Get_Struct(self, mpf_t, s);
	
	if (mpf_integer_p(*s))
		return Qtrue;
	else
		return Qfalse;
}
//// end of question-like methods
////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////
//// Other methods
// Sets the floating point precision for a specific number/object
// {GMP::Integer}, {Fixnum, Bignum} -> {NilClass}
static VALUE
f_set_precision( VALUE self, VALUE precision ) {
	// Creates a mpf_t pointer and loads self in it.
	// Also loads the precision into an unsigned long.
	mpf_t *s;
	unsigned long longPrecision = NUM2LONG(precision);
	Data_Get_Struct(self, mpf_t, s);
	
	// Sets the object's minimum precision
	mpf_set_prec(*s, longPrecision);
	
	return Qnil;
}

// Gets the floating point precision for a specific number/object.
// Careful with frequent access to this variable, since it needs to
// create a Fixnum object every time it's called.
// {} -> {Fixnum}
static VALUE
f_get_precision( VALUE self, VALUE precision ) {
	// Creates a mpf_t pointer and loads self in it.
	// Also creates a placeholder for the precision.
	mpf_t *s;
	unsigned long longPrecision = NUM2LONG(precision);
	Data_Get_Struct(self, mpf_t, s);
	
	// Sets the object's minimum precision
	longPrecision = mpf_get_prec(*s);
	
	return LONG2FIX(longPrecision);
}

// Efficient swap
// {GMP::Float}, {GMP::Float} -> {GMP::Float}, {GMP::Float}
static VALUE
f_swap( VALUE self, VALUE other ) {
	// Creates pointers to self's and other's mpf_t structures
	mpf_t *f, *o;
	
	// Copies back the mpf_t pointers wrapped in ruby data objects
	Data_Get_Struct(self, mpf_t, f);
	Data_Get_Struct(other, mpf_t, o);
	
	// Swaps the contents of self and other
	mpf_swap(*f, *o);
	
	return Qnil;
}

// Absolute value
// {} -> {GMP::Float}
static VALUE
f_absolute( VALUE self ) {
	// Creates pointers to self's and the result's mpf_t structures
	mpf_t *r = malloc(sizeof(*r));
	mpf_t *s;
	
	// Loads self from Ruby
	Data_Get_Struct(self, mpf_t, s);
	
	// Inits the result
	mpf_init(*r);
	
	// Sets the result as the absolute value of self
	mpf_abs(*r, *s);
	
	return Data_Wrap_Struct(cGMPFloat, float_mark, float_free, r);
}

// Relative difference ( |a - b|/a )
// {GMP::Float, GMP::Float} -> {GMP::Float}
static VALUE
f_relative_difference( VALUE self, VALUE other ) {
	// Creates pointers to self's, other's and result's mpf_t structures
	mpf_t *f, *o;
	mpf_t *r = malloc(sizeof(*r));
	
	// Copies back the mpf_t pointers from ruby to C
	Data_Get_Struct(self, mpf_t, f);
	Data_Get_Struct(other, mpf_t, o);
	
	// Inits the result
	mpf_init(*r);
	
	// Does the calculation
	mpf_reldiff(*r, *f, *o);
	
	return Data_Wrap_Struct(cGMPFloat, float_mark, float_free, r);
}

// Ceil (rounds up)
// {GMP::Float} -> {GMP::Float}
static VALUE
f_ceil( VALUE self ) {
	// Creates pointers to self's and the result's mpf_t structures
	mpf_t *r = malloc(sizeof(*r));
	mpf_t *s;
	
	// Loads self from Ruby
	Data_Get_Struct(self, mpf_t, s);
	
	// Inits the result
	mpf_init(*r);
	
	mpf_ceil(*r, *s);
	
	return Data_Wrap_Struct(cGMPFloat, float_mark, float_free, r);
}

// Floor (rounds down)
// {GMP::Float} -> {GMP::Float}
static VALUE
f_floor( VALUE self ) {
	// Creates pointers to self's and the result's mpf_t structures
	mpf_t *r = malloc(sizeof(*r));
	mpf_t *s;
	
	// Loads self from Ruby
	Data_Get_Struct(self, mpf_t, s);
	
	// Inits the result
	mpf_init(*r);
	
	mpf_floor(*r, *s);
	
	return Data_Wrap_Struct(cGMPFloat, float_mark, float_free, r);
}

// Truncate (rounds towards zero)
// {GMP::Float} -> {GMP::Float}
static VALUE
f_truncate( VALUE self ) {
	// Creates pointers to self's and the result's mpf_t structures
	mpf_t *r = malloc(sizeof(*r));
	mpf_t *s;
	
	// Loads self from Ruby
	Data_Get_Struct(self, mpf_t, s);
	
	// Inits the result
	mpf_init(*r);
	
	mpf_trunc(*r, *s);
	
	return Data_Wrap_Struct(cGMPFloat, float_mark, float_free, r);
}
//// end of other methods
////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////
//// Singletons/Class methods
// Sets the default floating point precision.
// GMP uses at least this number of bits.
// {Fixnum, Bignum} -> {NilClass}
static VALUE
f_set_def_prec( VALUE klass, VALUE precision ) {
	// Loads the number of bits into an unsigned long
	unsigned long longPrecision = NUM2LONG(precision);
	
	// Sets a class variable to hold this value
	rb_define_class_variable(cGMPFloat, "@@default_precision", precision);
	
	// Tells GMP to use it
	mpf_set_default_prec(longPrecision);
	
	return Qnil;
}

// Gets the default floating point precision.
// This should be used with care, since it creates a Fixnum every
// single time it's called.
// If you need frequent access to this value, consider using the
// class variable defined above.
// {} -> {Fixnum}
static VALUE
f_get_def_prec( VALUE klass ) {
	// Gets the precision from GMP
	unsigned long result = mpf_get_default_prec();
	
	return LONG2FIX(result);
}

// Square root
// {GMP::Float, Float} -> {GMP::Float}
static VALUE
f_sqrt_singleton( VALUE klass, VALUE radicand ) {
	// Creates pointer to the result's mpf_t structure
	mpf_t *r = malloc(sizeof(*r));
	
	// Inits the result
	mpf_init(*r);
	
	// Decides what to do based on the radicand's type/class
	switch (TYPE(radicand)) {
		case T_DATA: {
			if (rb_obj_class(radicand) == cGMPFloat) {
				mpf_t *rd;
				Data_Get_Struct(radicand, mpf_t, rd);
				if (mpf_sgn(*rd) == -1)
					rb_raise(rb_eRuntimeError, "radicand is negative");
				mpf_sqrt(*r, *rd);
			} else {
				rb_raise(rb_eTypeError, "radicand's class is not supported");
			}
			break;
		}
		case T_FLOAT: {
			mpf_t tempRafl;
			mpf_init_set_d(tempRafl, NUM2DBL(radicand));
			if (mpf_sgn(tempRafl) == -1)
				rb_raise(rb_eRuntimeError, "radicand is negative");
			mpf_sqrt(*r, tempRafl);
			mpf_clear(tempRafl);
			break;
		}
		default: {
			rb_raise(rb_eTypeError, "radicand's class is not supported");
		}
	}
	
	return Data_Wrap_Struct(cGMPFloat, float_mark, float_free, r);
}
//// end of singletons/class methods
////////////////////////////////////////////////////////////////////

#ifdef MPFR
////////////////////////////////////////////////////////////////////
//// Question-like methods
// Is it Not a Number?
// {} -> {TrueClass, FalseClass}
static VALUE
f_nan( VALUE self ) {
	// Loads self into a mpfr_t structure
	mpfr_t *s;
	Data_Get_Struct(self, mpfr_t, s);
	
	if (mpfr_nan_p(*s))
		return Qtrue;
	else
		return Qfalse;
}

// Is it infinity (well, not quite, but anyhow...)?
// {} -> {TrueClass, FalseClass}
static VALUE
f_inf( VALUE self ) {
	// Loads self into a mpfr_t structure
	mpfr_t *s;
	Data_Get_Struct(self, mpfr_t, s);
	
	if (mpfr_inf_p(*s))
		return Qtrue;
	else
		return Qfalse;
}

// Is it a number (i.e. neither NaN or infinity)
// {} -> {TrueClass, FalseClass}
static VALUE
f_number( VALUE self ) {
	// Loads self into a mpfr_t structure
	mpfr_t *s;
	Data_Get_Struct(self, mpfr_t, s);
	
	if (mpfr_number_p(*s))
		return Qtrue;
	else
		return Qfalse;
}

// Is it zero?
// {} -> {TrueClass, FalseClass}
static VALUE
f_zero( VALUE self ) {
	// Loads self into a mpfr_t structure
	mpfr_t *s;
	Data_Get_Struct(self, mpfr_t, s);
	
	if (mpfr_zero_p(*s))
		return Qtrue;
	else
		return Qfalse;
}
//// end of question-like methods
////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////
//// Trigonometric functions
// Sine
// {GMP::Float} -> {GMP::Float}
static VALUE
f_sine( VALUE klass, VALUE angle ) {
	// Creates pointers to the result's and angle's mpfr_t structures
	mpfr_t *r = malloc(sizeof(*r));
	mpfr_t *a;
	
	// Loads the angle from Ruby
	Data_Get_Struct(angle, mpfr_t, a);
	
	// Inits the result
	mpfr_init(*r);
	
	// Does the calculation
	mpfr_sin(*r, *a, GMP_RNDN);
	
	return Data_Wrap_Struct(cGMPFloat, float_mark, float_free, r);
}

// Cossine
// {GMP::Float} -> {GMP::Float}
static VALUE
f_cossine( VALUE klass, VALUE angle ) {
	// Creates pointers to the result's and angle's mpfr_t structures
	mpfr_t *r = malloc(sizeof(*r));
	mpfr_t *a;
	
	// Loads the angle from Ruby
	Data_Get_Struct(angle, mpfr_t, a);
	
	// Inits the result
	mpfr_init(*r);
	
	// Does the calculation
	mpfr_cos(*r, *a, GMP_RNDN);
	
	return Data_Wrap_Struct(cGMPFloat, float_mark, float_free, r);
}

// Tangent
// {GMP::Float} -> {GMP::Float}
static VALUE
f_tangent( VALUE klass, VALUE angle ) {
	// Creates pointers to the result's and angle's mpfr_t structures
	mpfr_t *r = malloc(sizeof(*r));
	mpfr_t *a;
	
	// Loads the angle from Ruby
	Data_Get_Struct(angle, mpfr_t, a);
	
	// Inits the result
	mpfr_init(*r);
	
	// Does the calculation
	mpfr_tan(*r, *a, GMP_RNDN);
	
	return Data_Wrap_Struct(cGMPFloat, float_mark, float_free, r);
}

// Cotangent
// {GMP::Float} -> {GMP::Float}
static VALUE
f_cotangent( VALUE klass, VALUE angle ) {
	// Creates pointers to the result's and angle's mpfr_t structures
	mpfr_t *r = malloc(sizeof(*r));
	mpfr_t *a;
	
	// Loads the angle from Ruby
	Data_Get_Struct(angle, mpfr_t, a);
	
	// Inits the result
	mpfr_init(*r);
	
	// Does the calculation
	mpfr_cot(*r, *a, GMP_RNDN);
	
	return Data_Wrap_Struct(cGMPFloat, float_mark, float_free, r);
}

// Secant
// {GMP::Float} -> {GMP::Float}
static VALUE
f_secant( VALUE klass, VALUE angle ) {
	// Creates pointers to the result's and angle's mpfr_t structures
	mpfr_t *r = malloc(sizeof(*r));
	mpfr_t *a;
	
	// Loads the angle from Ruby
	Data_Get_Struct(angle, mpfr_t, a);
	
	// Inits the result
	mpfr_init(*r);
	
	// Does the calculation
	mpfr_sec(*r, *a, GMP_RNDN);
	
	return Data_Wrap_Struct(cGMPFloat, float_mark, float_free, r);
}

// Cosecant
// {GMP::Float} -> {GMP::Float}
static VALUE
f_cosecant( VALUE klass, VALUE angle ) {
	// Creates pointers to the result's and angle's mpfr_t structures
	mpfr_t *r = malloc(sizeof(*r));
	mpfr_t *a;
	
	// Loads the angle from Ruby
	Data_Get_Struct(angle, mpfr_t, a);
	
	// Inits the result
	mpfr_init(*r);
	
	// Does the calculation
	mpfr_csc(*r, *a, GMP_RNDN);
	
	return Data_Wrap_Struct(cGMPFloat, float_mark, float_free, r);
}

// Array with sine and cossine
// {GMP::Float} -> {Array <<GMP::Float>> (2)
static VALUE
f_sine_and_cossine( VALUE klass, VALUE angle ) {
	// Creates pointers to the result's and angle's mpfr_t structures
	mpfr_t *s = malloc(sizeof(*s));
	mpfr_t *c = malloc(sizeof(*c));
	mpfr_t *a;
	
	// Loads the angle from Ruby
	Data_Get_Struct(angle, mpfr_t, a);
	
	// Inits the result
	mpfr_inits(*s, *c, (mpfr_ptr) 0);
	
	// Does the calculation
	mpfr_sin_cos(*s, *c, *a, GMP_RNDN);
	
	// Converts the mpz_t results to VALUEs
	VALUE sine = Data_Wrap_Struct(cGMPFloat, float_mark, float_free, s);
	VALUE cossine = Data_Wrap_Struct(cGMPFloat, float_mark, float_free, c);
	
	// Creates the resulting array
	VALUE array = rb_ary_new3(2, sine, cossine);
	
	return array;
}
//// end of trigonometric functions
////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////
//// Inverse trigonometric functions
// Inverse sine
// {GMP::Float} -> {GMP::Float}
static VALUE
f_asine( VALUE klass, VALUE trig_value ) {
	// Creates pointers to the result's and trig_value's mpfr_t structures
	mpfr_t *r = malloc(sizeof(*r));
	mpfr_t *a;
	
	// Loads the trig_value from Ruby
	Data_Get_Struct(trig_value, mpfr_t, a);
	
	// Inits the result
	mpfr_init(*r);
	
	// Does the calculation
	mpfr_asin(*r, *a, GMP_RNDN);
	
	return Data_Wrap_Struct(cGMPFloat, float_mark, float_free, r);
}

// Inverse cossine
// {GMP::Float} -> {GMP::Float}
static VALUE
f_acossine( VALUE klass, VALUE trig_value ) {
	// Creates pointers to the result's and trig_value's mpfr_t structures
	mpfr_t *r = malloc(sizeof(*r));
	mpfr_t *a;
	
	// Loads the trig_value from Ruby
	Data_Get_Struct(trig_value, mpfr_t, a);
	
	// Inits the result
	mpfr_init(*r);
	
	// Does the calculation
	mpfr_acos(*r, *a, GMP_RNDN);
	
	return Data_Wrap_Struct(cGMPFloat, float_mark, float_free, r);
}

// Inverse tangent
// {GMP::Float} -> {GMP::Float}
static VALUE
f_atangent( VALUE klass, VALUE trig_value ) {
	// Creates pointers to the result's and trig_value's mpfr_t structures
	mpfr_t *r = malloc(sizeof(*r));
	mpfr_t *a;
	
	// Loads the trig_value from Ruby
	Data_Get_Struct(trig_value, mpfr_t, a);
	
	// Inits the result
	mpfr_init(*r);
	
	// Does the calculation
	mpfr_atan(*r, *a, GMP_RNDN);
	
	return Data_Wrap_Struct(cGMPFloat, float_mark, float_free, r);
}

// Array with hyperbolic sine and cossine
// {GMP::Float} -> {Array <<GMP::Float>> (2)
static VALUE
f_hsine_and_hcossine( VALUE klass, VALUE angle ) {
	// Creates pointers to the result's and angle's mpfr_t structures
	mpfr_t *s = malloc(sizeof(*s));
	mpfr_t *c = malloc(sizeof(*c));
	mpfr_t *a;
	
	// Loads the angle from Ruby
	Data_Get_Struct(angle, mpfr_t, a);
	
	// Inits the result
	mpfr_inits(*s, *c, (mpfr_ptr) 0);
	
	// Does the calculation
	mpfr_sinh_cosh(*s, *c, *a, GMP_RNDN);
	
	// Converts the mpz_t results to VALUEs
	VALUE sine = Data_Wrap_Struct(cGMPFloat, float_mark, float_free, s);
	VALUE cossine = Data_Wrap_Struct(cGMPFloat, float_mark, float_free, c);
	
	// Creates the resulting array
	VALUE array = rb_ary_new3(2, sine, cossine);
	
	return array;
}
//// end of inverse trigonometric functions
////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////
//// Inverse hyperbolic trigonometry functions
// Inverse hyperbolic sine
// {GMP::Float} -> {GMP::Float}
static VALUE
f_ahsine( VALUE klass, VALUE trig_value ) {
	// Creates pointers to the result's and trig_value's mpfr_t structures
	mpfr_t *r = malloc(sizeof(*r));
	mpfr_t *a;
	
	// Loads the trig_value from Ruby
	Data_Get_Struct(trig_value, mpfr_t, a);
	
	// Inits the result
	mpfr_init(*r);
	
	// Does the calculation
	mpfr_asinh(*r, *a, GMP_RNDN);
	
	return Data_Wrap_Struct(cGMPFloat, float_mark, float_free, r);
}

// Inverse hyperbolic cossine
// {GMP::Float} -> {GMP::Float}
static VALUE
f_ahcossine( VALUE klass, VALUE trig_value ) {
	// Creates pointers to the result's and trig_value's mpfr_t structures
	mpfr_t *r = malloc(sizeof(*r));
	mpfr_t *a;
	
	// Loads the trig_value from Ruby
	Data_Get_Struct(trig_value, mpfr_t, a);
	
	// Inits the result
	mpfr_init(*r);
	
	// Does the calculation
	mpfr_acosh(*r, *a, GMP_RNDN);
	
	return Data_Wrap_Struct(cGMPFloat, float_mark, float_free, r);
}

// Inverse hyperbolic tangent
// {GMP::Float} -> {GMP::Float}
static VALUE
f_ahtangent( VALUE klass, VALUE trig_value ) {
	// Creates pointers to the result's and trig_value's mpfr_t structures
	mpfr_t *r = malloc(sizeof(*r));
	mpfr_t *a;
	
	// Loads the trig_value from Ruby
	Data_Get_Struct(trig_value, mpfr_t, a);
	
	// Inits the result
	mpfr_init(*r);
	
	// Does the calculation
	mpfr_atanh(*r, *a, GMP_RNDN);
	
	return Data_Wrap_Struct(cGMPFloat, float_mark, float_free, r);
}
//// end of inverse hyperbolic trigonometry functions
////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////
//// Hyperbolic trigonometry functions
// Hyperbolic sine
// {GMP::Float} -> {GMP::Float}
static VALUE
f_hsine( VALUE klass, VALUE angle ) {
	// Creates pointers to the result's and angle's mpfr_t structures
	mpfr_t *r = malloc(sizeof(*r));
	mpfr_t *a;
	
	// Loads the angle from Ruby
	Data_Get_Struct(angle, mpfr_t, a);
	
	// Inits the result
	mpfr_init(*r);
	
	// Does the calculation
	mpfr_sinh(*r, *a, GMP_RNDN);
	
	return Data_Wrap_Struct(cGMPFloat, float_mark, float_free, r);
}

// Hyperbolic cossine
// {GMP::Float} -> {GMP::Float}
static VALUE
f_hcossine( VALUE klass, VALUE angle ) {
	// Creates pointers to the result's and angle's mpfr_t structures
	mpfr_t *r = malloc(sizeof(*r));
	mpfr_t *a;
	
	// Loads the angle from Ruby
	Data_Get_Struct(angle, mpfr_t, a);
	
	// Inits the result
	mpfr_init(*r);
	
	// Does the calculation
	mpfr_cosh(*r, *a, GMP_RNDN);
	
	return Data_Wrap_Struct(cGMPFloat, float_mark, float_free, r);
}

// Hyperbolic tangent
// {GMP::Float} -> {GMP::Float}
static VALUE
f_htangent( VALUE klass, VALUE angle ) {
	// Creates pointers to the result's and angle's mpfr_t structures
	mpfr_t *r = malloc(sizeof(*r));
	mpfr_t *a;
	
	// Loads the angle from Ruby
	Data_Get_Struct(angle, mpfr_t, a);
	
	// Inits the result
	mpfr_init(*r);
	
	// Does the calculation
	mpfr_tanh(*r, *a, GMP_RNDN);
	
	return Data_Wrap_Struct(cGMPFloat, float_mark, float_free, r);
}

// Hyperbolic cotangent
// {GMP::Float} -> {GMP::Float}
static VALUE
f_hcotangent( VALUE klass, VALUE angle ) {
	// Creates pointers to the result's and angle's mpfr_t structures
	mpfr_t *r = malloc(sizeof(*r));
	mpfr_t *a;
	
	// Loads the angle from Ruby
	Data_Get_Struct(angle, mpfr_t, a);
	
	// Inits the result
	mpfr_init(*r);
	
	// Does the calculation
	mpfr_coth(*r, *a, GMP_RNDN);
	
	return Data_Wrap_Struct(cGMPFloat, float_mark, float_free, r);
}

// Hyperbolic secant
// {GMP::Float} -> {GMP::Float}
static VALUE
f_hsecant( VALUE klass, VALUE angle ) {
	// Creates pointers to the result's and angle's mpfr_t structures
	mpfr_t *r = malloc(sizeof(*r));
	mpfr_t *a;
	
	// Loads the angle from Ruby
	Data_Get_Struct(angle, mpfr_t, a);
	
	// Inits the result
	mpfr_init(*r);
	
	// Does the calculation
	mpfr_sech(*r, *a, GMP_RNDN);
	
	return Data_Wrap_Struct(cGMPFloat, float_mark, float_free, r);
}

// Hyperbolic cosecant
// {GMP::Float} -> {GMP::Float}
static VALUE
f_hcosecant( VALUE klass, VALUE angle ) {
	// Creates pointers to the result's and angle's mpfr_t structures
	mpfr_t *r = malloc(sizeof(*r));
	mpfr_t *a;
	
	// Loads the angle from Ruby
	Data_Get_Struct(angle, mpfr_t, a);
	
	// Inits the result
	mpfr_init(*r);
	
	// Does the calculation
	mpfr_csch(*r, *a, GMP_RNDN);
	
	return Data_Wrap_Struct(cGMPFloat, float_mark, float_free, r);
}
//// end of hyperbolic trigonometric functions
////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////
//// Logarithm methods
// Natural logarithm
// {GMP::Float} -> {GMP::Float}
// TODO: check the variable naming
static VALUE
f_logn( VALUE klass, VALUE logarithmand ) {
	// Creates pointers to the result's and logarithmand's mpfr_t structures
	mpfr_t *r = malloc(sizeof(*r));
	mpfr_t *l;
	
	// Loads the logarithmand from Ruby
	Data_Get_Struct(logarithmand, mpfr_t, l);
	
	// Inits the result;
	mpfr_init(*r);
	
	// Does the calculation
	mpfr_log(*r, *l, GMP_RNDN);
	
	return Data_Wrap_Struct(cGMPFloat, float_mark, float_free, r);
}

// Base 2 logarithm
// {GMP::Float} -> {GMP::Float}
// TODO: check the variable naming
static VALUE
f_log2( VALUE klass, VALUE logarithmand ) {
	// Creates pointers to the result's and logarithmand's mpfr_t structures
	mpfr_t *r = malloc(sizeof(*r));
	mpfr_t *l;
	
	// Loads the logarithmand from Ruby
	Data_Get_Struct(logarithmand, mpfr_t, l);
	
	// Inits the result;
	mpfr_init(*r);
	
	// Does the calculation
	mpfr_log2(*r, *l, GMP_RNDN);
	
	return Data_Wrap_Struct(cGMPFloat, float_mark, float_free, r);
}

// Base 10 logarithm
// {GMP::Float} -> {GMP::Float}
// TODO: check the variable naming
static VALUE
f_log10( VALUE klass, VALUE logarithmand ) {
	// Creates pointers to the result's and logarithmand's mpfr_t structures
	mpfr_t *r = malloc(sizeof(*r));
	mpfr_t *l;
	
	// Loads the logarithmand from Ruby
	Data_Get_Struct(logarithmand, mpfr_t, l);
	
	// Inits the result;
	mpfr_init(*r);
	
	// Does the calculation
	mpfr_log10(*r, *l, GMP_RNDN);
	
	return Data_Wrap_Struct(cGMPFloat, float_mark, float_free, r);
}
//// end of logarithm methods
////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////
//// Exponentiation methods
// Base E exponentiation
// {GMP::Float} -> {GMP::Float}
static VALUE
f_exp( VALUE klass, VALUE exponent ) {
	// Creates pointers to the result's and exponent's mpfr_t structures
	mpfr_t *r = malloc(sizeof(*r));
	mpfr_t *e;
	
	// Loads the exponent from Ruby
	Data_Get_Struct(exponent, mpfr_t, e);
	
	// Inits the result;
	mpfr_init(*r);
	
	// Does the calculation
	mpfr_exp(*r, *e, GMP_RNDN);
	
	return Data_Wrap_Struct(cGMPFloat, float_mark, float_free, r);
}

// Base 2 exponentiation
// {GMP::Float} -> {GMP::Float}
static VALUE
f_exp2( VALUE klass, VALUE exponent ) {
	// Creates pointers to the result's and exponent's mpfr_t structures
	mpfr_t *r = malloc(sizeof(*r));
	mpfr_t *e;
	
	// Loads the exponent from Ruby
	Data_Get_Struct(exponent, mpfr_t, e);
	
	// Inits the result;
	mpfr_init(*r);
	
	// Does the calculation
	mpfr_exp2(*r, *e, GMP_RNDN);
	
	return Data_Wrap_Struct(cGMPFloat, float_mark, float_free, r);
}

// Base 10 exponentiation
// {GMP::Float} -> {GMP::Float}
static VALUE
f_exp10( VALUE klass, VALUE exponent ) {
	// Creates pointers to the result's and exponent's mpfr_t structures
	mpfr_t *r = malloc(sizeof(*r));
	mpfr_t *e;
	
	// Loads the exponent from Ruby
	Data_Get_Struct(exponent, mpfr_t, e);
	
	// Inits the result;
	mpfr_init(*r);
	
	// Does the calculation
	mpfr_exp10(*r, *e, GMP_RNDN);
	
	return Data_Wrap_Struct(cGMPFloat, float_mark, float_free, r);
}
//// end of logarithm methods
////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////
//// Bessel functions
// Of the first kind and order 0
// {GMP::Float} -> {GMP::Float}
static VALUE
f_bessel_first_0( VALUE klass, VALUE number ) {
	// Creates pointer to the result's and number's mpfr_t structures
	mpfr_t *r = malloc(sizeof(*r));
	mpfr_t *n;
	
	// Loads the number from Ruby
	Data_Get_Struct(number, mpfr_t, n);
	
	// Inits the result;
	mpfr_init(*r);
	
	// Does the calculation
	mpfr_j0(*r, *n, GMP_RNDN);
	
	return Data_Wrap_Struct(cGMPFloat, float_mark, float_free, r);
}

// Of the first kind and order 1
// {GMP::Float} -> {GMP::Float}
static VALUE
f_bessel_first_1( VALUE klass, VALUE number ) {
	// Creates pointer to the result's and number's mpfr_t structures
	mpfr_t *r = malloc(sizeof(*r));
	mpfr_t *n;
	
	// Loads the number from Ruby
	Data_Get_Struct(number, mpfr_t, n);
	
	// Inits the result;
	mpfr_init(*r);
	
	// Does the calculation
	mpfr_j1(*r, *n, GMP_RNDN);
	
	return Data_Wrap_Struct(cGMPFloat, float_mark, float_free, r);
}

// Of the first kind and order n
// {GMP::Float}, {Fixnum} -> {GMP::Float}
static VALUE
f_bessel_first_n( VALUE klass, VALUE number, VALUE order ) {
	// Creates pointer to the result's and number's mpfr_t structures
	mpfr_t *r = malloc(sizeof(*r));
	long o;
	mpfr_t *n;
	
	// Loads both the number and the order from Ruby
	Data_Get_Struct(number, mpfr_t, n);
	o = FIX2LONG(order);
	
	// Inits the result;
	mpfr_init(*r);
	
	// Does the calculation
	mpfr_jn(*r, o, *n, GMP_RNDN);
	
	return Data_Wrap_Struct(cGMPFloat, float_mark, float_free, r);
}

// Of the second kind and order 0
// {GMP::Float} -> {GMP::Float}
static VALUE
f_bessel_second_0( VALUE klass, VALUE number ) {
	// Creates pointer to the result's and number's mpfr_t structures
	mpfr_t *r = malloc(sizeof(*r));
	mpfr_t *n;
	
	// Loads the number from Ruby
	Data_Get_Struct(number, mpfr_t, n);
	
	// Inits the result;
	mpfr_init(*r);
	
	// Does the calculation
	mpfr_y0(*r, *n, GMP_RNDN);
	
	return Data_Wrap_Struct(cGMPFloat, float_mark, float_free, r);
}

// Of the second kind and order 1
// {GMP::Float} -> {GMP::Float}
static VALUE
f_bessel_second_1( VALUE klass, VALUE number ) {
	// Creates pointer to the result's and number's mpfr_t structures
	mpfr_t *r = malloc(sizeof(*r));
	mpfr_t *n;
	
	// Loads the number from Ruby
	Data_Get_Struct(number, mpfr_t, n);
	
	// Inits the result;
	mpfr_init(*r);
	
	// Does the calculation
	mpfr_y1(*r, *n, GMP_RNDN);
	
	return Data_Wrap_Struct(cGMPFloat, float_mark, float_free, r);
}

// Of the second kind and order n
// {GMP::Float}, {Fixnum} -> {GMP::Float}
static VALUE
f_bessel_second_n( VALUE klass, VALUE number, VALUE order ) {
	// Creates pointer to the result's and number's mpfr_t structures
	mpfr_t *r = malloc(sizeof(*r));
	long o;
	mpfr_t *n;
	
	// Loads both the number and the order from Ruby
	Data_Get_Struct(number, mpfr_t, n);
	o = FIX2LONG(order);
	
	// Inits the result;
	mpfr_init(*r);
	
	// Does the calculation
	mpfr_yn(*r, o, *n, GMP_RNDN);
	
	return Data_Wrap_Struct(cGMPFloat, float_mark, float_free, r);
}
//// end of bessel functions
////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////
//// Other methods
// Factorial
// {Fixnum} -> {GMP::Float}
static VALUE
f_factorial( VALUE klass, VALUE base ) {
	// Creates pointer to the result's mpfr_t structure
	mpfr_t *r = malloc(sizeof(*r));
	unsigned long b = FIX2LONG(base);
	
	// Inits the result;
	mpfr_init(*r);
	
	// Does the calculation
	mpfr_fac_ui(*r, b, GMP_RNDN);
	
	return Data_Wrap_Struct(cGMPFloat, float_mark, float_free, r);
}

// Exponential integral of the input
// {GMP::Float} -> {GMP::Float}
static VALUE
f_exp_integral( VALUE klass, VALUE number ) {
	// Creates pointer to the result's and number's mpfr_t structures
	mpfr_t *r = malloc(sizeof(*r));
	mpfr_t *n;
	
	// Loads the number from Ruby
	Data_Get_Struct(number, mpfr_t, n);
	
	// Inits the result;
	mpfr_init(*r);
	
	// Does the calculation
	mpfr_eint(*r, *n, GMP_RNDN);
	
	return Data_Wrap_Struct(cGMPFloat, float_mark, float_free, r);
}

// Dilogarithm
// {GMP::Float} -> {GMP::Float}
static VALUE
f_dilogarithm( VALUE klass, VALUE number ) {
	// Creates pointer to the result's and number's mpfr_t structures
	mpfr_t *r = malloc(sizeof(*r));
	mpfr_t *n;
	
	// Loads the number from Ruby
	Data_Get_Struct(number, mpfr_t, n);
	
	// Inits the result;
	mpfr_init(*r);
	
	// Does the calculation
	mpfr_li2(*r, *n, GMP_RNDN);
	
	return Data_Wrap_Struct(cGMPFloat, float_mark, float_free, r);
}

// Euler gamma function
// {GMP::Float} -> {GMP::Float}
static VALUE
f_gamma( VALUE klass, VALUE number ) {
	// Creates pointer to the result's and number's mpfr_t structures
	mpfr_t *r = malloc(sizeof(*r));
	mpfr_t *n;
	
	// Loads the number from Ruby
	Data_Get_Struct(number, mpfr_t, n);
	
	// Inits the result;
	mpfr_init(*r);
	
	// Does the calculation
	mpfr_gamma(*r, *n, GMP_RNDN);
	
	return Data_Wrap_Struct(cGMPFloat, float_mark, float_free, r);
}

// Riemann zeta function
// {GMP::Float} -> {GMP::Float}
static VALUE
f_zeta( VALUE klass, VALUE number ) {
	// Creates pointer to the result's and number's mpfr_t structures
	mpfr_t *r = malloc(sizeof(*r));
	mpfr_t *n;
	
	// Loads the number from Ruby
	Data_Get_Struct(number, mpfr_t, n);
	
	// Inits the result;
	mpfr_init(*r);
	
	// Does the calculation
	mpfr_zeta(*r, *n, GMP_RNDN);
	
	return Data_Wrap_Struct(cGMPFloat, float_mark, float_free, r);
}

// Error function
// {GMP::Float} -> {GMP::Float}
static VALUE
f_error_function( VALUE klass, VALUE number ) {
	// Creates pointer to the result's and number's mpfr_t structures
	mpfr_t *r = malloc(sizeof(*r));
	mpfr_t *n;
	
	// Loads the number from Ruby
	Data_Get_Struct(number, mpfr_t, n);
	
	// Inits the result;
	mpfr_init(*r);
	
	// Does the calculation
	mpfr_erf(*r, *n, GMP_RNDN);
	
	return Data_Wrap_Struct(cGMPFloat, float_mark, float_free, r);
}
// Complimentary error function
// {GMP::Float} -> {GMP::Float}
static VALUE
f_error_function_comp( VALUE klass, VALUE number ) {
	// Creates pointer to the result's and number's mpfr_t structures
	mpfr_t *r = malloc(sizeof(*r));
	mpfr_t *n;
	
	// Loads the number from Ruby
	Data_Get_Struct(number, mpfr_t, n);
	
	// Inits the result;
	mpfr_init(*r);
	
	// Does the calculation
	mpfr_erfc(*r, *n, GMP_RNDN);
	
	return Data_Wrap_Struct(cGMPFloat, float_mark, float_free, r);
}

// Inverse square root (1/sqrt(x))
// {GMP::Float} -> {GMP::Float}
static VALUE
f_rec_sqrt( VALUE klass, VALUE radicand ) {
	// Creates pointer to the result's and number's mpfr_t structures
	mpfr_t *r = malloc(sizeof(*r));
	mpfr_t *n;
	
	// Loads the number from Ruby
	Data_Get_Struct(radicand, mpfr_t, n);
	
	// Checks that the radicand is positive
	if (mpfr_sgn(*n) <= 0)
		rb_raise(rb_eTypeError, "radicand must be positive");
	
	// Inits the result;
	mpfr_init(*r);
	
	// Does the calculation
	mpfr_rec_sqrt(*r, *n, GMP_RNDN);
	
	return Data_Wrap_Struct(cGMPFloat, float_mark, float_free, r);
}

// Cube root
// {GMP::Float} -> {GMP::Float}
static VALUE
f_cube_root( VALUE klass, VALUE radicand ) {
	// Creates pointer to the result's and number's mpfr_t structures
	mpfr_t *r = malloc(sizeof(*r));
	mpfr_t *n;
	
	// Loads the number from Ruby
	Data_Get_Struct(radicand, mpfr_t, n);
	
	// Inits the result;
	mpfr_init(*r);
	
	// Does the calculation
	mpfr_cbrt(*r, *n, GMP_RNDN);
	
	return Data_Wrap_Struct(cGMPFloat, float_mark, float_free, r);
}

// Nth root
// {GMP::Float}, {Fixnum} -> {GMP::Float}
static VALUE
f_nth_root( VALUE klass, VALUE radicand, VALUE degree ) {
	// Creates pointer to the result's and number's mpfr_t structures
	mpfr_t *r = malloc(sizeof(*r));
	mpfr_t *n;
	
	// Loads the number from Ruby
	Data_Get_Struct(radicand, mpfr_t, n);
	
	// Loads the degree from Ruby
	int intDegree = FIX2INT(degree);
	
	// Checks that the radicand is non-negative if the degree is even
	if (degree & 1 == 0 && mpfr_sgn(*n) < 0)
		rb_raise(rb_eTypeError, "radicand must be positive if degree is even");
	
	// Inits the result;
	mpfr_init(*r);
	
	// Does the calculation
	mpfr_root(*r, *n, intDegree, GMP_RNDN);
	
	return Data_Wrap_Struct(cGMPFloat, float_mark, float_free, r);
}

// Arithmetic-geometric mean
// {GMP::Float}, {GMP::Float} -> {GMP::Float}
static VALUE
f_ag_mean( VALUE klass, VALUE a, VALUE b ) {
	// Creates a pointer to the result
	mpfr_t *r = malloc(sizeof(*r));
	
	// Loads the numbers from Ruby
	mpfr_t *ma, *mb;
	Data_Get_Struct(a, mpfr_t, ma);
	Data_Get_Struct(b, mpfr_t, mb);
	
	// Inits the result;
	mpfr_init(*r);
	
	// Does the calculation
	mpfr_agm(*r, *ma, *mb, GMP_RNDN);
	
	return Data_Wrap_Struct(cGMPFloat, float_mark, float_free, r);
}

// Euclidean norm (also the hypotenuse of the corresponding right triangle)
// {GMP::Float}, {GMP::Float} -> {GMP::Float}
static VALUE
f_euclidean_norm( VALUE klass, VALUE a, VALUE b ) {
	// Creates a pointer to the result
	mpfr_t *r = malloc(sizeof(*r));
	
	// Loads the numbers from Ruby
	mpfr_t *ma, *mb;
	Data_Get_Struct(a, mpfr_t, ma);
	Data_Get_Struct(b, mpfr_t, mb);
	
	// Inits the result;
	mpfr_init(*r);
	
	// Does the calculation
	mpfr_hypot(*r, *ma, *mb, GMP_RNDN);
	
	return Data_Wrap_Struct(cGMPFloat, float_mark, float_free, r);
}
//// end of other methods
////////////////////////////////////////////////////////////////////
#endif // MPFR


void
Init_gmpf() {
	// Defines the module GMP and class GMP::Float
	mGMP = rb_define_module("GMP");
	cGMPFloat = rb_define_class_under(mGMP, "Float", rb_cObject);
	
	// Book keeping and the constructor method
	rb_define_alloc_func(cGMPFloat, float_allocate);
	rb_define_method(cGMPFloat, "initialize", f_init, -1);
	
	// Conversion methods
	rb_define_method(cGMPFloat, "to_s", f_to_string, 0);
	rb_define_method(cGMPFloat, "to_f", f_to_float, 0);
	
	// Binary operators
	rb_define_method(cGMPFloat, "+", f_addition, 1);
	rb_define_method(cGMPFloat, "-", f_subtraction, 1);
	rb_define_method(cGMPFloat, "*", f_multiplication, 1);
	rb_define_method(cGMPFloat, "/", f_division, 1);
	rb_define_method(cGMPFloat, "**", f_power, 1);
	
	// Unary operators
	rb_define_method(cGMPFloat, "+@", f_positive, 0);
	rb_define_method(cGMPFloat, "-@", f_negation, 0);
	
	// Comparisons
	rb_define_method(cGMPFloat, "==", f_equality_test, 1);
	rb_define_method(cGMPFloat, ">", f_greater_than_test, 1);
	rb_define_method(cGMPFloat, "<", f_less_than_test, 1);
	rb_define_method(cGMPFloat, ">=", f_greater_than_or_equal_to_test, 1);
	rb_define_method(cGMPFloat, "<=", f_less_than_or_equal_to_test, 1);
	rb_define_method(cGMPFloat, "<=>", f_generic_comparison, 1);
	
	// Question-like methods
	rb_define_method(cGMPFloat, "is_integer?", f_integer, 0);
	
	// Other methods
	rb_define_method(cGMPFloat, "precision=", f_set_precision, 1);
	rb_define_method(cGMPFloat, "precision", f_get_precision, 0);
	rb_define_method(cGMPFloat, "swap", f_swap, 1);
	rb_define_method(cGMPFloat, "abs", f_absolute, 0);
	rb_define_method(cGMPFloat, "relative_diff", f_relative_difference, 1);
	rb_define_method(cGMPFloat, "ceil", f_ceil, 0);
	rb_define_method(cGMPFloat, "floor", f_floor, 0);
	rb_define_method(cGMPFloat, "trunc", f_truncate, 0);
	
	// Singletons/Class methods
	rb_define_singleton_method(cGMPFloat, "def_precision=", f_set_def_prec, 1);
	rb_define_singleton_method(cGMPFloat, "def_precision", f_get_def_prec, 0);
	rb_define_singleton_method(cGMPFloat, "sqrt", f_sqrt_singleton, 1);
	
#ifdef MPFR
	// Question-like methods
	rb_define_method(cGMPFloat, "nan?", f_nan, 0);
	rb_define_method(cGMPFloat, "inf?", f_inf, 0);
	rb_define_method(cGMPFloat, "number?", f_number, 0);
	rb_define_method(cGMPFloat, "zero?", f_zero, 0);
	
	// Trigonometric functions
	rb_define_singleton_method(cGMPFloat, "sin", f_sine, 1);
	rb_define_singleton_method(cGMPFloat, "cos", f_cossine, 1);
	rb_define_singleton_method(cGMPFloat, "tan", f_tangent, 1);
	rb_define_singleton_method(cGMPFloat, "cot", f_cotangent, 1);
	rb_define_singleton_method(cGMPFloat, "sec", f_secant, 1);
	rb_define_singleton_method(cGMPFloat, "csc", f_cosecant, 1);
	rb_define_singleton_method(cGMPFloat, "sin_cos", f_sine_and_cossine, 1);
	
	// Hyperbolic trigonometry functions
	rb_define_singleton_method(cGMPFloat, "sinh", f_hsine, 1);
	rb_define_singleton_method(cGMPFloat, "cosh", f_hcossine, 1);
	rb_define_singleton_method(cGMPFloat, "tanh", f_htangent, 1);
	rb_define_singleton_method(cGMPFloat, "coth", f_hcotangent, 1);
	rb_define_singleton_method(cGMPFloat, "sech", f_hsecant, 1);
	rb_define_singleton_method(cGMPFloat, "csch", f_hcosecant, 1);
	rb_define_singleton_method(cGMPFloat, "sinh_cosh", f_hsine_and_hcossine, 1);
	
	// Inverse trigonometric functions
	rb_define_singleton_method(cGMPFloat, "asin", f_asine, 1);
	rb_define_singleton_method(cGMPFloat, "acos", f_acossine, 1);
	rb_define_singleton_method(cGMPFloat, "atan", f_atangent, 1);
	
	// Inverse hyperbolic trigonometry functions
	rb_define_singleton_method(cGMPFloat, "asinh", f_ahsine, 1);
	rb_define_singleton_method(cGMPFloat, "acosh", f_ahcossine, 1);
	rb_define_singleton_method(cGMPFloat, "atanh", f_ahtangent, 1);
	
	// Logarithm methods
	rb_define_singleton_method(cGMPFloat, "log", f_logn, 1);
	rb_define_singleton_method(cGMPFloat, "log2", f_log2, 1);
	rb_define_singleton_method(cGMPFloat, "log10", f_log10, 1);
	
	// Exponentiation methods
	rb_define_singleton_method(cGMPFloat, "exp", f_exp, 1);
	rb_define_singleton_method(cGMPFloat, "exp2", f_exp2, 1);
	rb_define_singleton_method(cGMPFloat, "exp10", f_exp10, 1);
	
	// Bessel functions
	rb_define_singleton_method(cGMPFloat, "j0", f_bessel_first_0, 1);
	rb_define_singleton_method(cGMPFloat, "j1", f_bessel_first_1, 1);
	rb_define_singleton_method(cGMPFloat, "jn", f_bessel_first_n, 2);
	rb_define_singleton_method(cGMPFloat, "y0", f_bessel_second_0, 1);
	rb_define_singleton_method(cGMPFloat, "y1", f_bessel_second_1, 1);
	rb_define_singleton_method(cGMPFloat, "yn", f_bessel_second_n, 2);
	
	// Other methods
	rb_define_singleton_method(cGMPFloat, "fac", f_factorial, 1);
	rb_define_singleton_method(cGMPFloat, "eint", f_exp_integral, 1);
	rb_define_singleton_method(cGMPFloat, "li2", f_dilogarithm, 1);
	rb_define_singleton_method(cGMPFloat, "gamma", f_gamma, 1);
	rb_define_singleton_method(cGMPFloat, "zeta", f_zeta, 1);
	rb_define_singleton_method(cGMPFloat, "erf", f_error_function, 1);
	rb_define_singleton_method(cGMPFloat, "erfc", f_error_function_comp, 1);
	rb_define_singleton_method(cGMPFloat, "rec_sqrt", f_rec_sqrt, 1);
	rb_define_singleton_method(cGMPFloat, "cbrt", f_cube_root, 1);
	rb_define_singleton_method(cGMPFloat, "root", f_nth_root, 2);
	rb_define_singleton_method(cGMPFloat, "agm", f_ag_mean, 2);
	rb_define_singleton_method(cGMPFloat, "hypot", f_euclidean_norm, 2);
#endif // MPFR

	// Aliases
	rb_define_alias(cGMPFloat, "magnitude", "abs");
}
