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
