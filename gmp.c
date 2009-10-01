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

static VALUE mGMP;
static VALUE cGMPInteger;

////////////////////////////////////////////////////////////////////
//// Fundamental methods
// Garbage collection
static void
integer_mark( mpz_t *i ) {}

static void
integer_free( mpz_t *i ) {
	mpz_clear(*i);
}

// Object allocation
static VALUE
integer_allocate( VALUE klass ) {
	mpz_t *i = malloc(sizeof(mpz_t));
	mpz_init(*i);
	return Data_Wrap_Struct(klass, integer_mark, integer_free, i);
}

// Class constructor
static VALUE
init( VALUE self, VALUE intData ) {
	// Creates a mpz_t pointer and loads self in it
	mpz_t *i;
	Data_Get_Struct(self, mpz_t, i);
	
	switch (TYPE(intData)) {
		case T_STRING: {
			mpz_set_str(*i, StringValuePtr(intData), 10);
			break;
		}
		case T_FIXNUM: {
			mpz_set_ui(*i, FIX2LONG(intData));
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
static VALUE
to_string( VALUE self ) {
	// Creates a mpz_t pointer and loads self in it
	mpz_t *i;
	Data_Get_Struct(self, mpz_t, i);
	
	char *intStr = (char *) malloc(mpz_sizeinbase(*i, 10) + 2);
	
	mpz_get_str(intStr, 10, *i);
	
	return rb_str_new2(intStr);
}

// To Fixnum/Bignum
static VALUE
to_integer( VALUE self ) {
	// Creates a mpz_t pointer and loads self in it
	mpz_t *i;
	Data_Get_Struct(self, mpz_t, i);
	
	signed long tempLong = mpz_get_si(*i);
	
	return LONG2FIX(tempLong);
}

// To Float (double-precision floating point number)
static VALUE
to_float( VALUE self ) {
	// Creates a mpz_t pointer and loads self in it
	mpz_t *i;
	Data_Get_Struct(self, mpz_t, i);
	
	double tempDouble = mpz_get_d(*i);
	
	return rb_float_new(tempDouble);
}
//// end of fundamental methods
////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////
//// Binary arithmetical operators (operations taking two values)
// Addition (+)
// {GMP::Integer, Fixnum} -> {GMP::Integer}
static VALUE
addition( VALUE self, VALUE summand ) {
	// Creates pointers to self's and the result's mpz_t structures
	mpz_t *i, *r;
	
	// Creates a new object that will receive the result of the addition
	VALUE argv[] = { rb_str_new2("0") };
	ID class_id = rb_intern("Integer");
	VALUE class = rb_const_get(mGMP, class_id);
	VALUE result = rb_class_new_instance(1, argv, class);
	
	// Copies back the mpz_t pointers from ruby to C
	Data_Get_Struct(self, mpz_t, i);
	Data_Get_Struct(result, mpz_t, r);
	
	// Decides what to do based on the multiplicand's type/class
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
			unsigned long sl = FIX2LONG(summand);
			mpz_add_ui(*r, *i, sl);
			break;
		}
		default: {
			rb_raise(rb_eTypeError, "input data type not supported");
		}
	}
	
	return result;
}

// Subtraction (-)
// {GMP::Integer, Fixnum} -> {GMP::Integer}
static VALUE
subtraction( VALUE self, VALUE subtraend ) {
	// Creates pointers to self's and the result's mpz_t structures
	mpz_t *i, *r;
	
	// Creates a new object that will receive the result of the operation
	VALUE argv[] = { rb_str_new2("0") };
	ID class_id = rb_intern("Integer");
	VALUE class = rb_const_get(mGMP, class_id);
	VALUE result = rb_class_new_instance(1, argv, class);
	
	// Copies back the mpz_t pointers wrapped in ruby data objects
	Data_Get_Struct(self, mpz_t, i);
	Data_Get_Struct(result, mpz_t, r);
	
	// Decides what to do based on the multiplicand's type/class
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
			unsigned long sl = FIX2LONG(subtraend);
			mpz_sub_ui(*r, *i, sl);
			break;
		}
		default: {
			rb_raise(rb_eTypeError, "input data type not supported");
		}
	}
	
	return result;
}

// Multiplication (*)
// {GMP::Integer, Fixnum} -> {GMP::Integer}
static VALUE
multiplication( VALUE self, VALUE multiplicand ) {
	// Creates pointers to self's and the result's mpz_t structures
	mpz_t *i, *r;
	
	// Creates a new object which will receive the result from the operation
	// TODO: put this in its own function
	VALUE argv[] = { rb_str_new2("0") };
	ID class_id = rb_intern("Integer");
	VALUE class = rb_const_get(mGMP, class_id);
	VALUE result = rb_class_new_instance(1, argv, class);
	
	// Copies back the mpz_t pointers wrapped in ruby data objects
	Data_Get_Struct(self, mpz_t, i);
	Data_Get_Struct(result, mpz_t, r);
	
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
			unsigned long sl = FIX2LONG(multiplicand);
			mpz_mul_ui(*r, *i, sl);
			break;
		}
		default: {
			rb_raise(rb_eTypeError, "input data type not supported");
		}
	}
	
	return result;
}

// Division (/)
// {GMP::Integer, Fixnum} -> {GMP::Integer}
static VALUE
division( VALUE self, VALUE dividend ) {
	// Creates pointers to self's and the result's mpz_t structures
	mpz_t *i, *r;
	
	// Creates a new object that will receive the result of the addition
	VALUE argv[] = { rb_str_new2("0") };
	ID class_id = rb_intern("Integer");
	VALUE class = rb_const_get(mGMP, class_id);
	VALUE result = rb_class_new_instance(1, argv, class);
	
	// Copies back the mpz_t pointers from ruby to C
	Data_Get_Struct(self, mpz_t, i);
	Data_Get_Struct(result, mpz_t, r);
	
	// Decides what to do based on the multiplicand's type/class
	switch (TYPE(dividend)) {
		case T_DATA: {
			if (rb_obj_class(dividend) == cGMPInteger) {
				mpz_t *sd;
				Data_Get_Struct(dividend, mpz_t, sd);
				if (mpz_cmp_ui(*sd, (unsigned long) 0) == 0)
					rb_raise(rb_eRuntimeError, "divided by 0");
				mpz_fdiv_q(*r, *i, *sd);
			} else {
				rb_raise(rb_eTypeError, "input data type not supported");
			}
			break;
		}
		case T_FIXNUM: {
			unsigned long sl = FIX2LONG(dividend);
			if (sl == (unsigned long) 0)
				rb_raise(rb_eRuntimeError, "divided by 0");
			mpz_fdiv_q_ui(*r, *i, sl);
			break;
		}
		default: {
			rb_raise(rb_eTypeError, "input data type not supported");
		}
	}
	
	return result;
}

// Modulo (from modular arithmetic) (%)
// {GMP::Integer, Fixnum} -> {GMP::Integer}
static VALUE
modulo( VALUE self, VALUE base ) {
	// Creates pointers to self's and the result's mpz_t structures
	mpz_t *i, *r;
	
	// Creates a new object which will receive the result from the operation
	// TODO: put this in its own function
	VALUE argv[] = { rb_str_new2("0") };
	ID class_id = rb_intern("Integer");
	VALUE class = rb_const_get(mGMP, class_id);
	VALUE result = rb_class_new_instance(1, argv, class);
	
	// Copies back the mpz_t pointers wrapped in ruby data objects
	Data_Get_Struct(self, mpz_t, i);
	Data_Get_Struct(result, mpz_t, r);
	
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
	
	return result;
}

// Exponetiation (**)
// {Fixnum} -> {GMP::Integer}
static VALUE
power( VALUE self, VALUE exp ) {
	// Creates pointers to self's and the result's mpz_t structures
	mpz_t *i, *r;
	
	// Creates a new object which will receive the result from the operation
	// TODO: put this in its own function
	VALUE argv[] = { rb_str_new2("0") };
	ID class_id = rb_intern("Integer");
	VALUE class = rb_const_get(mGMP, class_id);
	VALUE result = rb_class_new_instance(1, argv, class);
	
	// Copies back the mpz_t pointers wrapped in ruby data objects
	Data_Get_Struct(self, mpz_t, i);
	Data_Get_Struct(result, mpz_t, r);
	
	// Decides what to do based on the exponent's type/class
	switch (TYPE(exp)) {
		case T_DATA: {
			mpz_t *sd;
			Data_Get_Struct(exp, mpz_t, sd);
			// TODO
			rb_raise(rb_eRuntimeError, "not working yet");
			break;
		}
		case T_FIXNUM: {
			unsigned long sl = FIX2LONG(exp);
			mpz_pow_ui(*r, *i, sl);
			break;
		}
		default: {
			rb_raise(rb_eTypeError, "exponent's type not supported");
		}
	}
	
	return result;
}
//// end of binary operator methods
////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////
//// Unary arithmetical operators (operations over a single value)
// Plus (+a)
// {GMP::Integer} -> {GMP::Integer}
static VALUE
positive( VALUE self ) {
	return self;
}

// Negation (-a)
// {GMP::Integer} -> {GMP::Integer}
static VALUE
negation( VALUE self ) {
	// Creates pointers to self's and the result's mpz_t structures
	mpz_t *i, *r;
	
	// Creates a new object which will receive the result from the operation
	// TODO: put this in its own function
	VALUE argv[] = { rb_str_new2("0") };
	ID class_id = rb_intern("Integer");
	VALUE class = rb_const_get(mGMP, class_id);
	VALUE result = rb_class_new_instance(1, argv, class);
	
	// Copies back the mpz_t pointers wrapped in ruby data objects
	Data_Get_Struct(self, mpz_t, i);
	Data_Get_Struct(result, mpz_t, r);
	
	// Negates i, and copies the result to r
	mpz_neg(*r, *i);
	
	return result;
}
//// end of unary operators methods
////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////
//// Logic operators
// Logic AND (&)
// {GMP::Integer} -> {GMP::Integer}
static VALUE
logic_and( VALUE self, VALUE other ) {
	// Creates pointers to self's, other's and result's mpz_t structures
	mpz_t *i, *o, *r;
	
	// Creates a new object which will receive the result from the operation
	// TODO: put this in its own function
	VALUE argv[] = { rb_str_new2("0") };
	ID class_id = rb_intern("Integer");
	VALUE class = rb_const_get(mGMP, class_id);
	VALUE result = rb_class_new_instance(1, argv, class);
	
	// Copies back the mpz_t pointers wrapped in ruby data objects
	Data_Get_Struct(self, mpz_t, i);
	Data_Get_Struct(other, mpz_t, o);
	Data_Get_Struct(result, mpz_t, r);
	
	// Sets result as self bitwise-and other
	mpz_and(*r, *i, *o);
	
	return result;
}

// Logic OR (inclusive OR) (|)
// {GMP::Integer} -> {GMP::Integer}
static VALUE
logic_ior( VALUE self, VALUE other ) {
	// Creates pointers to self's, other's and result's mpz_t structures
	mpz_t *i, *o, *r;
	
	// Creates a new object which will receive the result from the operation
	// TODO: put this in its own function
	VALUE argv[] = { rb_str_new2("0") };
	ID class_id = rb_intern("Integer");
	VALUE class = rb_const_get(mGMP, class_id);
	VALUE result = rb_class_new_instance(1, argv, class);
	
	// Copies back the mpz_t pointers wrapped in ruby data objects
	Data_Get_Struct(self, mpz_t, i);
	Data_Get_Struct(other, mpz_t, o);
	Data_Get_Struct(result, mpz_t, r);
	
	// Sets result as self bitwise inclusive-or other
	mpz_ior(*r, *i, *o);
	
	return result;
}

// Logic XOR (exclusive OR) (^)
// {GMP::Integer} -> {GMP::Integer}
static VALUE 
logic_xor( VALUE self, VALUE other ) {
	// Creates pointers to self's, other's and result's mpz_t structures
	mpz_t *i, *o, *r;
	
	// Creates a new object which will receive the result from the operation
	// TODO: put this in its own function
	VALUE argv[] = { rb_str_new2("0") };
	ID class_id = rb_intern("Integer");
	VALUE class = rb_const_get(mGMP, class_id);
	VALUE result = rb_class_new_instance(1, argv, class);
	
	// Copies back the mpz_t pointers wrapped in ruby data objects
	Data_Get_Struct(self, mpz_t, i);
	Data_Get_Struct(other, mpz_t, o);
	Data_Get_Struct(result, mpz_t, r);
	
	// Sets result as self bitwise exclusive-or other
	mpz_xor(*r, *i, *o);
	
	return result;
}

// Logic NOT (~)
// {GMP::Integer} -> {GMP::Integer}
static VALUE
logic_not( VALUE self ) {
	// Creates pointers to self's and the result's mpz_t structures
	mpz_t *i, *r;
	
	// Creates a new object which will receive the result from the operation
	// TODO: put this in its own function
	VALUE argv[] = { rb_str_new2("0") };
	ID class_id = rb_intern("Integer");
	VALUE class = rb_const_get(mGMP, class_id);
	VALUE result = rb_class_new_instance(1, argv, class);
	
	// Copies back the mpz_t pointers wrapped in ruby data objects
	Data_Get_Struct(self, mpz_t, i);
	Data_Get_Struct(result, mpz_t, r);
	
	// Sets result as the one's complement of self
	mpz_com(*r, *i);
	
	return result;
}
//// end of logic manipulation methods
////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////
//// Comparison methods
// Equality (==)
// {GMP::Integer} -> {TrueClass, FalseClass}
static VALUE
equality_test( VALUE self, VALUE other ) {
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

// Greater than (>)
// {GMP::Integer} -> {TrueClass, FalseClass}
static VALUE
greater_than_test( VALUE self, VALUE other ) {
	// Creates pointers to self's and the other's mpz_t structures
	mpz_t *i, *o;
	
	// Copies back the mpz_t pointers wrapped in ruby data objects
	Data_Get_Struct(self, mpz_t, i);
	Data_Get_Struct(other, mpz_t, o);
	
	if (mpz_cmp(*i, *o) > 0)
		return Qtrue;
	else
		return Qfalse;
}

// Less than (<)
// {GMP::Integer} -> {TrueClass, FalseClass}
static VALUE
less_than_test( VALUE self, VALUE other ) {
	// Creates pointers to self's and the other's mpz_t structures
	mpz_t *i, *o;
	
	// Copies back the mpz_t pointers wrapped in ruby data objects
	Data_Get_Struct(self, mpz_t, i);
	Data_Get_Struct(other, mpz_t, o);
	
	if (mpz_cmp(*i, *o) < 0)
		return Qtrue;
	else
		return Qfalse;
}

// Greater than or equal to (>=)
// {GMP::Integer} -> {TrueClass, FalseClass}
static VALUE
greater_than_or_equal_to_test( VALUE self, VALUE other ) {
	// Creates pointers to self's and the other's mpz_t structures
	mpz_t *i, *o;
	
	// Copies back the mpz_t pointers wrapped in ruby data objects
	Data_Get_Struct(self, mpz_t, i);
	Data_Get_Struct(other, mpz_t, o);
	
	if (mpz_cmp(*i, *o) >= 0)
		return Qtrue;
	else
		return Qfalse;
}

// Less than or equal to (<=)
// {GMP::Integer} -> {TrueClass, FalseClass}
static VALUE
less_than_or_equal_to_test( VALUE self, VALUE other ) {
	// Creates pointers to self's and the other's mpz_t structures
	mpz_t *i, *o;
	
	// Copies back the mpz_t pointers wrapped in ruby data objects
	Data_Get_Struct(self, mpz_t, i);
	Data_Get_Struct(other, mpz_t, o);
	
	if (mpz_cmp(*i, *o) <= 0)
		return Qtrue;
	else
		return Qfalse;
}

// Generic comparison (<=>)
// {GMP::Integer} -> {Fixnum}
static VALUE
generic_comparison( VALUE self, VALUE other ) {
	// Creates pointers to self's and the other's mpz_t structures
	mpz_t *i, *o;
	
	// Copies back the mpz_t pointers wrapped in ruby data objects
	Data_Get_Struct(self, mpz_t, i);
	Data_Get_Struct(other, mpz_t, o);
	
	return INT2FIX(mpz_cmp(*i, *o));
}

//// end of comparison methods
////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////
//// Inplace methods (the caller inherits the result)
// (probably) Sets self as the next prime greater than itself
// {GMP::Integer} -> {GMP::Integer}
static VALUE
next_prime_inplace( VALUE self ) {
	// Creates a mpz_t pointer and loads self in it
	mpz_t *i;
	Data_Get_Struct(self, mpz_t, i);
	
	mpz_nextprime(*i, *i);
	
	return Qnil;
}

// Absolute value
// {GMP::Integer} -> {GMP::Integer}
static VALUE
absolute_inplace( VALUE self ) {
	// Creates a mpz_t pointer and loads self in it
	mpz_t *i;
	Data_Get_Struct(self, mpz_t, i);
	
	mpz_abs(*i, *i);
	
	return Qnil;
}

// Negation
// {GMP::Integer} -> {GMP::Integer}
static VALUE
negation_inplace( VALUE self ) {
	// Creates a mpz_t pointer and loads self in it
	mpz_t *i;
	Data_Get_Struct(self, mpz_t, i);
	
	mpz_neg(*i, *i);
	
	return Qnil;
}

// Square root (instance method)
// {GMP::Integer} -> {GMP::Integer}
static VALUE
sqrt_inplace( VALUE self ) {
	// Creates a mpz_t pointer and loads self in it
	mpz_t *i;
	Data_Get_Struct(self, mpz_t, i);
	
	mpz_sqrt(*i, *i);
	
	return Qnil;
}

// Nth root (instance method)
// {GMP::Integer} -> {GMP::Integer}
static VALUE
root_inplace( VALUE self, VALUE degree ) {
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
//// end of inplace methods
////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////
//// Question-like methods
static VALUE
divisible( VALUE self, VALUE base ) {
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

static VALUE
perfect_power( VALUE self ) {
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

static VALUE
perfect_square( VALUE self ) {
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

static VALUE
probable_prime( VALUE self, VALUE numberOfTests ) {
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

static VALUE
even( VALUE self ) {
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

static VALUE
odd( VALUE self ) {
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
//// end of question-like methods
////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////
//// Other operations
static VALUE
absolute( VALUE self ) {
	// Creates pointers to self's and the result's mpz_t structures
	mpz_t *i, *r;
	
	// Creates a new object which will receive the result from the operation
	// TODO: put this in its own function
	VALUE argv[] = { rb_str_new2("0") };
	ID class_id = rb_intern("Integer");
	VALUE class = rb_const_get(mGMP, class_id);
	VALUE result = rb_class_new_instance(1, argv, class);
	
	// Copies back the mpz_t pointers wrapped in ruby data objects
	Data_Get_Struct(self, mpz_t, i);
	Data_Get_Struct(result, mpz_t, r);
	
	// Sets the result as the absolute value of self
	mpz_abs(*r, *i);
	
	return result;
}

static VALUE
next_prime( VALUE self ) {
	// Creates pointers to self's and the result's mpz_t structures
	mpz_t *i, *r;
	
	// Creates a new object which will receive the result from the operation
	// TODO: put this in its own function
	VALUE argv[] = { rb_str_new2("0") };
	ID class_id = rb_intern("Integer");
	VALUE class = rb_const_get(mGMP, class_id);
	VALUE result = rb_class_new_instance(1, argv, class);
	
	// Copies back the mpz_t pointers wrapped in ruby data objects
	Data_Get_Struct(self, mpz_t, i);
	Data_Get_Struct(result, mpz_t, r);
	
	// (probably) Sets the result as the next prime greater than itself
	mpz_nextprime(*r, *i);
	
	return result;
}

// Number of digits in a specific base
// {GMP::Integer}, {Fixnum} -> {Fixnum}
static VALUE
size_in_base( VALUE self, VALUE base ) {
	// TODO: check why 2 has size 2 in base 3
	
	// Creates a mpz_t pointer and loads self in it
	mpz_t *i;
	Data_Get_Struct(self, mpz_t, i);
	
	// Converts the base from Fixnum to int and checks if its valid
	int intBase = FIX2INT(base);
	if (intBase < 2 || intBase > 62)
		rb_raise(rb_eRuntimeError, "base out of range");
	
	unsigned int size = (unsigned int) mpz_sizeinbase(*i, intBase);
	
	return INT2FIX(size);
}

// Efficient swap
// {GMP::Integer}, {GMP::Integer} -> {GMP::Integer}, {GMP::Integer}
static VALUE
swap( VALUE self, VALUE other ) {
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
static VALUE
next( VALUE self ) {
	// Creates pointers to self's and the result's mpz_t structures
	mpz_t *i, *r;
	
	// Creates a new object which will receive the result from the operation
	// TODO: put this in its own function
	VALUE argv[] = { rb_str_new2("0") };
	ID class_id = rb_intern("Integer");
	VALUE class = rb_const_get(mGMP, class_id);
	VALUE result = rb_class_new_instance(1, argv, class);
	
	// Copies back the mpz_t pointers wrapped in ruby data objects
	Data_Get_Struct(self, mpz_t, i);
	Data_Get_Struct(result, mpz_t, r);
	
	// Sets the result as the next number from self
	mpz_add_ui(*r, *i, (unsigned long) 1);
	
	return result;
}
//// end of other operations
////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////
//// Singletons/Class methods
// Exponetiation with modulo (powermod)
// {GMP::Integer, Fixnum} -> {GMP::Integer}
static VALUE
powermod( VALUE klass, VALUE self, VALUE exp, VALUE base ) {
	// Creates pointers to self's, base's and result's mpz_t structures
	mpz_t *i, *r;
	
	// Creates a new object which will receive the result from the operation
	// TODO: put this in its own function
	VALUE argv[] = { rb_str_new2("0") };
	ID class_id = rb_intern("Integer");
	VALUE class = rb_const_get(mGMP, class_id);
	VALUE result = rb_class_new_instance(1, argv, class);
	
	// Copies back the mpz_t pointers wrapped in ruby data objects
	Data_Get_Struct(self, mpz_t, i);
	Data_Get_Struct(result, mpz_t, r);
	
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
	
	return result;
}

// Square root
// {GMP::Integer} -> {GMP::Integer}
static VALUE
sqrt_singleton( VALUE klass, VALUE number ) {
	// Creates pointers to number's and the result's mpz_t structures
	mpz_t *n, *r;
	
	// Creates a new object which will receive the result from the operation
	// TODO: put this in its own function
	VALUE argv[] = { rb_str_new2("0") };
	ID class_id = rb_intern("Integer");
	VALUE class = rb_const_get(mGMP, class_id);
	VALUE result = rb_class_new_instance(1, argv, class);
	
	// Copies back the mpz_t pointers wrapped in ruby data objects
	Data_Get_Struct(number, mpz_t, n);
	Data_Get_Struct(result, mpz_t, r);
	
	// Checks if the number is positive, raising an exception if not
	if (mpz_sgn(*n) == -1)
		rb_raise(rb_eRuntimeError, "number is negative");
	
	// Takes the floor of the square root
	mpz_sqrt(*r, *n);
	
	return result;
}

// Nth root
// {GMP::Integer}, {Fixnum} -> {GMP::Integer}
static VALUE
root_singleton( VALUE klass, VALUE number, VALUE degree ) {
	// Creates pointers to number's and the result's mpz_t structures
	// Also, creates the degree placeholder
	mpz_t *n, *r;
	unsigned long longDegree;
	
	// Creates a new object which will receive the result from the operation
	// TODO: put this in its own function
	VALUE argv[] = { rb_str_new2("0") };
	ID class_id = rb_intern("Integer");
	VALUE class = rb_const_get(mGMP, class_id);
	VALUE result = rb_class_new_instance(1, argv, class);
	
	// Copies back the mpz_t pointers wrapped in ruby data objects
	// as well as the root value
	Data_Get_Struct(number, mpz_t, n);
	Data_Get_Struct(result, mpz_t, r);
	longDegree = NUM2LONG(degree);
	
	// If the degree is zero, GMP will normally give a floating point error
	// A possible solution is to make the degree -1, which works as expected
	if (longDegree == (unsigned long) 0)
		--longDegree;
	
	// Raise an exception if both the number is negative and the
	// degree is even
	if (mpz_sgn(*n) == -1 && longDegree % 2 == 0)
		rb_raise(rb_eRuntimeError, "number is negative, but degree is even");
	
	mpz_root(*r, *n, longDegree);
	
	return result;
}

// Fibonacci numbers generator
// {Fixnum} -> {GMP::Integer}
static VALUE
fibonacci_singleton( VALUE klass, VALUE index ) {
	// Creates pointers to the result's mpz_t structure, and loads
	// creates a placeholder for the index
	mpz_t *r;
	unsigned long longIndex;
	
	// Creates a new object which will receive the result from the operation
	// TODO: put this in its own function
	VALUE argv[] = { rb_str_new2("0") };
	ID class_id = rb_intern("Integer");
	VALUE class = rb_const_get(mGMP, class_id);
	VALUE result = rb_class_new_instance(1, argv, class);
	
	// Copies back the mpz_t pointer wrapped in a ruby data object
	// as well as the index
	Data_Get_Struct(result, mpz_t, r);
	longIndex = NUM2LONG(index);
	
	mpz_fib_ui(*r, longIndex);
	
	return result;
}

static VALUE
lucas_singleton( VALUE klass, VALUE index ) {
	// Creates pointers to the result's mpz_t structure, and loads
	// creates a placeholder for the index
	mpz_t *r;
	unsigned long longIndex;
	
	// Creates a new object which will receive the result from the operation
	// TODO: put this in its own function
	VALUE argv[] = { rb_str_new2("0") };
	ID class_id = rb_intern("Integer");
	VALUE class = rb_const_get(mGMP, class_id);
	VALUE result = rb_class_new_instance(1, argv, class);
	
	// Copies back the mpz_t pointer wrapped in a ruby data object
	// as well as the index
	Data_Get_Struct(result, mpz_t, r);
	longIndex = NUM2LONG(index);
	
	mpz_lucnum_ui(*r, longIndex);
	
	return result;
}

static VALUE
factorial_singleton( VALUE klass, VALUE index ) {
	// Creates pointers to the result's mpz_t structure, and
	// creates a placeholder for the index
	mpz_t *r;
	unsigned long longIndex;
	
	// Creates a new object which will receive the result from the operation
	// TODO: put this in its own function
	VALUE argv[] = { rb_str_new2("0") };
	ID class_id = rb_intern("Integer");
	VALUE class = rb_const_get(mGMP, class_id);
	VALUE result = rb_class_new_instance(1, argv, class);
	
	// Copies back the mpz_t pointer wrapped in a ruby data object
	// as well as the index
	Data_Get_Struct(result, mpz_t, r);
	longIndex = NUM2LONG(index);
	
	mpz_fac_ui(*r, longIndex);
	
	return result;
}

// TODO: check whether N and K have names
static VALUE
binomial_singleton( VALUE klass, VALUE n, VALUE k ) {
	// Checks whether k has a valid type
	if (TYPE(k) != T_FIXNUM)
		rb_raise(rb_eTypeError, "k is not of a supported data type");
	
	// Creates pointers to the result's mpz_t structure and creates
	// a placeholder for k
	mpz_t *r;
	unsigned long longK;
	
	// Creates a new object which will receive the result from the operation
	// TODO: put this in its own function
	VALUE argv[] = { rb_str_new2("0") };
	ID class_id = rb_intern("Integer");
	VALUE class = rb_const_get(mGMP, class_id);
	VALUE result = rb_class_new_instance(1, argv, class);
	
	// Copies back the mpz_t pointer wrapped in a ruby data object
	// as well as the index
	Data_Get_Struct(result, mpz_t, r);
	longK = NUM2LONG(k);
	
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
	
	return result;
}

// TODO: decide how to handle mpz_remove's output
static VALUE
remove_singleton( VALUE klass, VALUE number, VALUE factor ) {
	// Creates pointers to the number's, factor's and result's mpz_t
	// structures
	mpz_t *n, *f, *r;
	
	// Creates a new object which will receive the result from the operation
	// TODO: put this in its own function
	VALUE argv[] = { rb_str_new2("0") };
	ID class_id = rb_intern("Integer");
	VALUE class = rb_const_get(mGMP, class_id);
	VALUE result = rb_class_new_instance(1, argv, class);
	
	// Copies back the mpz_t pointers wrapped in ruby data objects
	Data_Get_Struct(number, mpz_t, n);
	Data_Get_Struct(factor, mpz_t, f);
	Data_Get_Struct(result, mpz_t, r);
	
	mpz_remove(*r, *n, *f);
	
	return result;
}
//// end of singleton/class methods
////////////////////////////////////////////////////////////////////




void
Init_gmp() {
	// Defines the module GMP and class GMP::Integer
	mGMP = rb_define_module("GMP");
	cGMPInteger = rb_define_class_under(mGMP, "Integer", rb_cObject);
	
	// Book keeping and the constructor method
	rb_define_alloc_func(cGMPInteger, integer_allocate);
	rb_define_method(cGMPInteger, "initialize", init, 1);
	
	// Converters
	rb_define_method(cGMPInteger, "to_s", to_string, 0);
	rb_define_method(cGMPInteger, "to_i", to_integer, 0);
	rb_define_method(cGMPInteger, "to_f", to_float, 0);
	
	// Binary operators
	rb_define_method(cGMPInteger, "+", addition, 1);
	rb_define_method(cGMPInteger, "-", subtraction, 1);
	rb_define_method(cGMPInteger, "*", multiplication, 1);
	rb_define_method(cGMPInteger, "/", division, 1);
	rb_define_method(cGMPInteger, "%", modulo, 1);
	rb_define_method(cGMPInteger, "**", power, 1);
	
	// Unary operators
	rb_define_method(cGMPInteger, "+@", positive, 0);
	rb_define_method(cGMPInteger, "-@", negation, 0);
	
	// Logic operators
	rb_define_method(cGMPInteger, "&", logic_and, 1);
	rb_define_method(cGMPInteger, "|", logic_ior, 1);
	rb_define_method(cGMPInteger, "^", logic_xor, 1);
	rb_define_method(cGMPInteger, "~", logic_not, 0);
	
	// Comparisons
	rb_define_method(cGMPInteger, "==", equality_test, 1);
	rb_define_method(cGMPInteger, ">", greater_than_test, 1);
	rb_define_method(cGMPInteger, "<", less_than_test, 1);
	rb_define_method(cGMPInteger, ">=", greater_than_or_equal_to_test, 1);
	rb_define_method(cGMPInteger, "<=", less_than_or_equal_to_test, 1);
	rb_define_method(cGMPInteger, "<=>", generic_comparison, 1);
	
	// Inplace methods
	// TODO: define non-inplace functions based on their inplace equivalents?
	rb_define_method(cGMPInteger, "abs!", absolute_inplace, 0);
	rb_define_method(cGMPInteger, "neg!", negation_inplace, 0);
	rb_define_method(cGMPInteger, "next_prime!", next_prime_inplace, 0);
	rb_define_method(cGMPInteger, "sqrt!", sqrt_inplace, 0);
	rb_define_method(cGMPInteger, "root!", root_inplace, 1);
	
	// Question-like methods
	rb_define_method(cGMPInteger, "divisible_by?", divisible, 1);
	rb_define_method(cGMPInteger, "perfect_power?", perfect_power, 0);
	rb_define_method(cGMPInteger, "perfect_square?", perfect_square, 0);
	rb_define_method(cGMPInteger, "probable_prime?", probable_prime, 1);
	rb_define_method(cGMPInteger, "even?", even, 0);
	rb_define_method(cGMPInteger, "odd?", odd, 0);
	
	// Other operations
	rb_define_method(cGMPInteger, "abs", absolute, 0);
	rb_define_method(cGMPInteger, "next_prime", next_prime, 0);
	rb_define_method(cGMPInteger, "size_in_base", size_in_base, 1);
	rb_define_method(cGMPInteger, "swap", swap, 1);
	rb_define_method(cGMPInteger, "next", next, 0);
	
	// Singletons/Class methods
	rb_define_singleton_method(cGMPInteger, "powermod", powermod, 3);
	rb_define_singleton_method(cGMPInteger, "sqrt", sqrt_singleton, 1);
	rb_define_singleton_method(cGMPInteger, "root", root_singleton, 2);
	rb_define_singleton_method(cGMPInteger, "fib", fibonacci_singleton, 1);
	rb_define_singleton_method(cGMPInteger, "luc", lucas_singleton, 1);
	rb_define_singleton_method(cGMPInteger, "fac", factorial_singleton, 1);
	rb_define_singleton_method(cGMPInteger, "bin", binomial_singleton, 2);
	rb_define_singleton_method(cGMPInteger, "remove", remove_singleton, 2);
}
