AUTOMAKE_OPTIONS = foreign

bin_PROGRAMS = prs_db_builder prs_server

prs_db_builder_CFLAGS = \
	$(vorbis_CFLAGS) \
	$(vorbisfile_CFLAGS) \
	$(libxml2_CFLAGS) \
	$(sqlite3_CFLAGS)
prs_db_builder_LDADD = \
	$(pthread_LIBS) \
	$(vorbis_LIBS) \
	$(vorbisfile_LIBS) \
	$(libxml2_LIBS) \
	$(taglib_c_LIBS) \
	$(sqlite3_LIBS)
prs_db_builder_SOURCES = \
	db.c \
	fileinfo.c \
	mp3fileinfo.c \
	vorbisfileinfo.c \
	wavefileinfo.c \
	mp3decoder.c \
	mp3header.c \
	list.c	\
	debug.c \
	prs_db_builder.c

prs_server_CFLAGS = \
	$(vorbis_CFLAGS) \
	$(vorbisfile_CFLAGS) \
	$(libxml2_CFLAGS) \
	$(shout_CFLAGS) \
	$(libcurl_CFLAGS) \
	$(sqlite3_CFLAGS) \
	$(alsa_CFLAGS)
prs_server_LDADD = \
	$(pthread_LIBS) \
	$(vorbis_LIBS) \
	$(vorbisfile_LIBS) \
	$(libxml2_LIBS) \
	$(shout_LIBS) \
	$(libcurl_LIBS) \
	$(taglib_c_LIBS) \
	$(sqlite3_LIBS) \
	$(alsa_LIBS)
prs_server_SOURCES = \
	fileinfo.c \
	fileinfo.h \
	vorbisfileinfo.c \
	vorbisfileinfo.h \
	mp3fileinfo.c \
	mp3fileinfo.h \
	wavefileinfo.c \
	wavefileinfo.h \
	wave.h \
	db.c \
	db.h \
	list.c \
	list.h \
	prs.c \
	prs.h \
	prs_config.c \
	prs_config.h \
	mixer.c \
	mixer.h \
	mixerbus.c \
	mixerbus.h \
	mixeroutput.c \
	mixeroutput.h \
	alsamixeroutput.c \
	alsamixeroutput.h \
	aplaymixeroutput.c \
	aplaymixeroutput.h \
	ossmixeroutput.c \
	ossmixeroutput.h \
	soundcard.c \
	soundcard.h \
	shoutmixeroutput.c \
	shoutmixeroutput.h \
	filemixeroutput.c \
	filemixeroutput.h \
	mixerpatchpoint.c \
	mixerpatchpoint.h \
	resample.c \
	resample.h \
	mixerchannel.c \
	mixerchannel.h \
	vorbismixerchannel.c \
	vorbismixerchannel.h \
	mp3mixerchannel.c \
	mp3mixerchannel.h \
	mp3decoder.c \
	mp3decoder.h \
	mp3header.c \
	mp3header.h \
	ossmixerchannel.c \
	ossmixerchannel.h \
	urlmixerchannel.c \
	urlmixerchannel.h \
	wavemixerchannel.c \
	wavemixerchannel.h \
	audiofilter.c \
	audiofilter.h \
	SimpleComp.cpp \
	SimpleComp.h \
	SimpleCompProcess.inl \
	SimpleEnvelope.cpp \
	SimpleEnvelope.h \
	SimpleGain.h \
	SimpleHeader.h \
	audiocompressor.cpp \
	audiocompressor.h \
	multibandaudiocompressor.c \
	multibandaudiocompressor.h \
	mixerautomation.c \
	mixerautomation.h \
	logger.c \
	logger.h \
	scheduler.c \
	scheduler.h \
	debug.c \
	debug.h \
	prs_server.c
