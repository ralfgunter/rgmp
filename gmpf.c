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
	
	return rb_str_new2(str);
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
	mpf_t *f, *r;
	
	// Creates a new object that will receive the result of the addition
	VALUE argv[] = { rb_float_new(0.0) };
	ID class_id = rb_intern("Float");
	VALUE class = rb_const_get(mGMP, class_id);
	VALUE result = rb_class_new_instance(1, argv, class);
	
	// Copies back the mpf_t pointers from ruby to C
	Data_Get_Struct(self, mpf_t, f);
	Data_Get_Struct(result, mpf_t, r);
	
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
	
	return result;
}

// Subtraction (-)
// {GMP::Float, Float, Fixnum, Bignum} -> {GMP::Float}
static VALUE
f_subtraction( VALUE self, VALUE subtraend ) {
	// Creates pointers to self's and the result's mpf_t structures
	mpf_t *f, *r;
	
	// Creates a new object that will receive the result of the subtraction
	VALUE argv[] = { rb_float_new(0.0) };
	ID class_id = rb_intern("Float");
	VALUE class = rb_const_get(mGMP, class_id);
	VALUE result = rb_class_new_instance(1, argv, class);
	
	// Copies back the mpf_t pointers from ruby to C
	Data_Get_Struct(self, mpf_t, f);
	Data_Get_Struct(result, mpf_t, r);
	
	// Decides what to do based on the summand's type/class
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
	
	return result;
}

// Multiplication (*)
// {GMP::Float, Float, Fixnum, Bignum} -> {GMP::Float}
static VALUE
f_multiplication( VALUE self, VALUE multiplicand ) {
	// Creates pointers to self's and the result's mpf_t structures
	mpf_t *f, *r;
	
	// Creates a new object that will receive the result of the subtraction
	VALUE argv[] = { rb_float_new(0.0) };
	ID class_id = rb_intern("Float");
	VALUE class = rb_const_get(mGMP, class_id);
	VALUE result = rb_class_new_instance(1, argv, class);
	
	// Copies back the mpf_t pointers from ruby to C
	Data_Get_Struct(self, mpf_t, f);
	Data_Get_Struct(result, mpf_t, r);
	
	// Decides what to do based on the summand's type/class
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
			// GMP does not provide support for adding a mpf_t to a C double
			// so first we have to convert Ruby's Float to a mpf_t and then do
			// the operation
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
	
	return result;
}
//// end of conversion methods
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
	mpf_t *f, *r;
	
	// Creates a new object that will receive the result of the subtraction
	VALUE argv[] = { rb_float_new(0.0) };
	ID class_id = rb_intern("Float");
	VALUE class = rb_const_get(mGMP, class_id);
	VALUE result = rb_class_new_instance(1, argv, class);
	
	// Copies back the mpf_t pointers from ruby to C
	Data_Get_Struct(self, mpf_t, f);
	Data_Get_Struct(result, mpf_t, r);
	
	// Negates i, and copies the result to r
	mpf_neg(*r, *f);
	
	return result;
}
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
//// end of singletons/class methods
////////////////////////////////////////////////////////////////////
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
	
	// Binary operators
	rb_define_method(cGMPFloat, "+", f_addition, 1);
	rb_define_method(cGMPFloat, "-", f_subtraction, 1);
	rb_define_method(cGMPFloat, "*", f_multiplication, 1);
	
	// Unary operators
	rb_define_method(cGMPFloat, "+@", f_positive, 0);
	rb_define_method(cGMPFloat, "-@", f_negation, 0);
	
	// Other methods
	rb_define_method(cGMPFloat, "precision=", f_set_precision, 1);
	rb_define_method(cGMPFloat, "precision", f_get_precision, 0);
	
	// Singletons/Class methods
	rb_define_singleton_method(cGMPFloat, "def_precision=", f_set_def_prec, 1);
	rb_define_singleton_method(cGMPFloat, "def_precision", f_get_def_prec, 0);
}
