
noinst_LIBRARIES = libetg.a

libetg_a_SOURCES = \
	adm_gate.C adm_hookup.C d3des.C d3encdec.C d3test.C \
	adm_gate.H adm_hookup.H d3des.H d3encdec.H packet.H \
	packet_decoder.C packet_encoder.C tunnel.C \
	packet_decoder.H packet_encoder.H tunnel.H \
	main_genkey.C main_proxy.C main_worker.C packet.C

AM_CPPFLAGS = \
	-I$(top_srcdir)/libpfkutil \
	-I$(top_srcdir)/libpfkdll2 \
	-I$(top_builddir)/main
