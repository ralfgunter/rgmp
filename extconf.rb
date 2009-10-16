require 'mkmf'

dir_config('gmp')
dir_config('mpfr')

if have_library('mpfr')
	$CFLAGS += ' -DMPFR'
else
	raise "Missing MPFR library"
end

if not have_library('gmp')
	raise "Missing GMP library"
end

create_makefile('gmp')
