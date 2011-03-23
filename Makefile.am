ACLOCAL_AMFLAGS = -I m4

bin_PROGRAMS = mandelbulb

mandelbulb_SOURCES = \
	src/core/bounds.h \
	src/core/conffile.cpp src/core/conffile.h \
	src/core/display.cpp src/core/display.h \
	src/core/extensions.cpp src/core/extensions.h \
	src/core/fxfont.cpp src/core/fxfont.h \
	src/core/logger.cpp src/core/logger.h \
	src/core/mousecursor.cpp src/core/mousecursor.h \
	src/core/pi.h \
	src/core/regex.cpp src/core/regex.h \
	src/core/resource.cpp src/core/resource.h \
	src/core/sdlapp.cpp src/core/sdlapp.h \
	src/core/seeklog.cpp src/core/seeklog.h \
	src/core/settings.cpp src/core/settings.h \
	src/core/shader.cpp src/core/shader.h \
	src/core/stringhash.cpp src/core/stringhash.h \
	src/core/texture.cpp src/core/texture.h \
	src/core/vectors.h \
	src/ppm.cpp src/ppm.h \
	src/vcamera.cpp src/vcamera.h \
	src/viewer_settings.cpp src/viewer_settings.h \
	src/viewer.cpp src/viewer.h

CPPFLAGS = -DSDLAPP_RESOURCE_DIR=\"$(pkgdatadir)\"

dist_pkgdata_DATA = data/cursor.png data/shaders/MandelbulbQuick.frag data/shaders/MandelbulbQuick.vert

CPPFLAGS += -DSDLAPP_SHADER_SUPPORT=1

if FONTDIR
CPPFLAGS += -DSDLAPP_FONT_DIR=\"$(sdlappfontdir)\"
else
fontsdir = $(pkgdatadir)/fonts
dist_fonts_DATA = data/fonts/README data/fonts/FreeSans.ttf
endif

