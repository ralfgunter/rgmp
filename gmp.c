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

#include "rgmp.h"

VALUE mGMP;
VALUE cGMPInteger, cGMPRational, cGMPFloat;
VALUE gmpversion, mpfrversion;

void
Init_gmp() {
	mGMP = rb_define_module("GMP");
	
	cGMPInteger = rb_define_class_under(mGMP, "Integer", rb_cObject);
	cGMPRational = rb_define_class_under(mGMP, "Rational", rb_cObject);
	cGMPFloat = rb_define_class_under(mGMP, "Float", rb_cObject);
	
	// Loads GMP::Integer into the extension
	Init_gmpz();
	
	// Loads GMP::Rational into the extension
	Init_gmpq();
	
	// Loads GMP::Float into the extension
	Init_gmpf();
	
	// String containing the GMP version used to compile this
	gmpversion = rb_str_new2(gmp_version);
	rb_define_const(mGMP, "GMP_VERSION", gmpversion);
	
#ifdef MPFR
	mpfrversion = rb_str_new2(MPFR_VERSION_STRING);
#else
	mpfrversion = Qnil;
#endif

	// If MPFR was present during compile-time, this constant holds its version,
	// otherwise it is set as Nil.
	rb_define_const(mGMP, "MPFR_VERSION", mpfrversion);
}
