require 'mkmf'

extension_name = 'gmp'
dir_config(extension_name)
if have_library('gmp')
	$CFLAGS += ' -lgmp'
else
	raise "Missing GMP library"
end
create_makefile(extension_name)
