
noinst_LIBRARIES = libbackup.a
libbackup_a_SOURCES = \
	FileList.cc FileList.h \
	create_backup.cc create_file.cc update_backup.cc \
	database_elements.h params.h protos.h \
	delete_backup.cc delete_gen.cc delete_gens.cc \
	extract.cc extract_list.cc \
	find_backup.cc get_info.cc list_backups.cc list_files.cc \
	md5buffer.cc main.cc

AM_CPPFLAGS = \
	-I$(top_srcdir)/libpfkdll2 \
	-I$(top_srcdir)/libpfkutil \
	-I$(top_srcdir)/libpfkfb
