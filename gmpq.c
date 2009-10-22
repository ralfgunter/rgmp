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
static VALUE cGMPRational;

////////////////////////////////////////////////////////////////////
//// Fundamental methods
// Garbage collection
static void
rational_mark( mpq_t *q ) {}

static void
rational_free( mpq_t *q ) {
	mpq_clear(*q);
}

// Object allocation
static VALUE
rational_allocate( VALUE klass ) {
	mpq_t *q = malloc(sizeof(mpq_t));
	mpq_init(*q);
	return Data_Wrap_Struct(klass, rational_mark, rational_free, q);
}

// Class constructor
static VALUE
q_init( VALUE self, VALUE ratData ) {
	// Creates a mpq_t pointer and loads self into it.
	mpq_t *q;
	Data_Get_Struct(self, mpq_t, q);
	
	switch (TYPE(ratData)) {
		case T_STRING: {
			mpq_set_str(*q, StringValuePtr(ratData), 10);
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
// To string
// {} -> {String}
static VALUE
q_to_string( VALUE argc, VALUE *argv, VALUE self ) {
	// Loads self into a mpq_t
	mpq_t *s;
	Data_Get_Struct(self, mpq_t, s);
	
	// Creates a placeholder for the optional argument
	VALUE base;
	
	// The base argument is optional, and can vary from 2 to 36
	rb_scan_args(argc, argv, "01", &base);
	
	// Loads the base into an int, regardless of it having been passed or not
	if (!FIXNUM_P(base) && !NIL_P(base))
		rb_raise(rb_eTypeError, "base must be a fixnum");
	int intBase = FIX2INT(base);
	
	// If the base hasn't been defined, this defaults it to 10
	if (base == Qnil)
		intBase = 10;
	
	// Checks if the base is within range
	if (!(intBase >= 2 && intBase <= 36))
		rb_raise(rb_eRangeError, "base out of range");
	
	char *intStr = mpq_get_str(NULL, intBase, *s);
	VALUE rStr = rb_str_new2(intStr);
	free(intStr);
	
	return rStr;
}
//// end of conversion methods
////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////
//// Unary arithmetical operators
// Plus (+a)
// {GMP::Rational} -> {GMP::Rational}
static VALUE
q_positive( VALUE self ) {
	return self;
}

// Negation (-a)
// {GMP::Rational} -> {GMP::Rational}
static VALUE
q_negation( VALUE self ) {
	// Creates pointers to self's and the result's mpq_t structures
	mpq_t *r = malloc(sizeof(*r));
	mpq_t *q;
	
	// Loads self into *f
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
//// Other operations
// Efficient swap
// {GMP::Rational} -> {GMP::Rational}
static VALUE
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
static VALUE
q_sign( VALUE self ) {
	// Loads self into a mpq_t
	mpq_t *s;
	Data_Get_Struct(self, mpq_t, s);
	
	// Gets the sign of self
	int intSign = mpq_sgn(*s);
	
	return INT2FIX(intSign);
}
//// end of other operations
////////////////////////////////////////////////////////////////////

void
Init_gmpq() {
	// Defines the module GMP and class GMP::Rational
	mGMP = rb_define_module("GMP");
	cGMPRational = rb_define_class_under(mGMP, "Rational", rb_cObject);
	
	// Book keeping and the constructor method
	rb_define_alloc_func(cGMPRational, rational_allocate);
	rb_define_method(cGMPRational, "initialize", q_init, 1);
	
	// Conversion methods
	rb_define_method(cGMPRational, "to_s", q_to_string, -1);
	
	// Unary arithmetical operators
	rb_define_method(cGMPRational, "+@", q_positive, 0);
	rb_define_method(cGMPRational, "-@", q_negation, 0);
	
	// Other operations
	rb_define_method(cGMPRational, "swap", q_swap, 1);
	rb_define_method(cGMPRational, "sign", q_sign, 0);
}
