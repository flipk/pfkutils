
noinst_LIBRARIES = libbackup.a
libbackup_a_SOURCES = \
	FileList.C FileList.H \
	create_backup.C create_file.C update_backup.C \
	database_elements.H params.H protos.H \
	delete_backup.C delete_gen.C delete_gens.C \
	extract.C extract_list.C \
	find_backup.C get_info.C list_backups.C list_files.C \
	md5buffer.C main.C

AM_CPPFLAGS = \
	-I$(top_srcdir)/libpfkdll2 \
	-I$(top_srcdir)/libpfkutil \
	-I$(top_srcdir)/libpfkfb
