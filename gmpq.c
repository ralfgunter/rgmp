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



void
Init_gmpq() {
	// Defines the module GMP and class GMP::Rational
	mGMP = rb_define_module("GMP");
	cGMPRational = rb_define_class_under(mGMP, "Rational", rb_cObject);
	
	// Book keeping and the constructor method
	rb_define_alloc_func(cGMPRational, rational_allocate);
	rb_define_method(cGMPRational, "initialize", q_init, 1);
}
