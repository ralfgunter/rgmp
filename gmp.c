#include "rgmp.h"

VALUE mGMP;

VALUE cGMPInteger, cGMPFloat;

void
Init_gmp() {
	mGMP = rb_define_module("GMP");
	
	cGMPInteger = rb_define_class_under(mGMP, "Integer", rb_cObject);
	cGMPFloat = rb_define_class_under(mGMP, "Float", rb_cObject);
	
	// Load GMP::Integer into the extension
	Init_gmpz();
	
	// Load GMP::Float into the extension
	Init_gmpf();
	
	// String containing the GMP version used to compile this
	VALUE version = rb_str_new2(gmp_version);
	rb_define_const(mGMP, "GMP_VERSION", version);
}
