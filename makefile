EXENAME=6player
DEFINES=$(options) HAVE_W32API_H __WXMSW__ _UNICODE WXUSINGDLL NOPCH

ifeq ($(OS),Windows_NT)
EXT_EXE=.exe
EXT_DLL=.dll
else
EXT_EXE=
EXT_DLL=.so
endif

ifeq ($(mode),release)
NAME_SUFFIX=
DEFINES += RELEASE
CXXOPTFLAGS=-s -O3
else
NAME_SUFFIX=d
DEFINES += DEBUG
CXXOPTFLAGS=-g
endif

EXECUTABLE=$(EXENAME)$(NAME_SUFFIX)$(EXT_EXE)
OBJDIR=obj$(NAME_SUFFIX)/

CXX=g++
GCC=gcc
WINDRES=windres
WINDRESFLAGS=$(addprefix -D,$(DEFINES)) -I"$(CPATH)"
CXXFLAGS=-std=gnu++17 -Wextra $(addprefix -D,$(DEFINES)) -mthreads
LDFLAGS=-lwxbase33u$(NAME_SUFFIX) -lwxmsw33u$(NAME_SUFFIX)_core -lws2_32 -L. -lbass -lbass_fx -lbassmidi -lbassmix -lbassenc -lbassenc_mp3 -lbassenc_ogg -lbassenc_flac -lbassenc_opus -lbassenc_aac -lUniversalSpeech -liphlpapi -lole32 -loleaut32 -loleacc -lfmt -larchive -mthreads -mthreads -mwindows

SRCS=$(wildcard src/app/*.cpp) $(wildcard src/caster/*.cpp) $(wildcard src/common/*.cpp) $(wildcard src/effect/*.cpp) $(wildcard src/encoder/*.cpp) $(wildcard src/loader/*.cpp) $(wildcard src/playlist/*.cpp)
RCSRCS=$(wildcard src/app/*.rc)
BASS_OPENMPT_SRCS=$(wildcard src/bassopenmpt/*.c)
BASS_SNDFILE_SRCS=$(wildcard src/bass_sndfile/*.c)
BASS_GME_SRCS=$(wildcard src/bass_gme/*.c)
BASS_HVL_SRCS=$(wildcard src/bass_hvl/*.c)
BASS_MDX_SRCS=$(wildcard src/bass_mdxmini/*.c)
BASS_VSTIMIDI_SRCS=$(wildcard src/bassvstimidi/*.cpp)
BASS_ADPLUG_SRCS=$(wildcard src/bass_opl/*.cpp)
BASS_YM_SRCS=$(wildcard src/bass_ym/*.c)

OBJS=$(addprefix $(OBJDIR),$(SRCS:.cpp=.o))
RCOBJS=$(addprefix $(OBJDIR)rsrc/,$(RCSRCS:.rc=.o))

PERCENT=%

all: $(EXECUTABLE)

.PHONY: $(EXECUTABLE)

clean:
	rm -r $(OBJDIR)

$(EXECUTABLE): $(RCOBJS) $(OBJS)
	$(CXX) $(CXXFLAGS) $(CXXOPTFLAGS) -o $@ $^ $(LDFLAGS)

$(OBJDIR)%.o: %.cpp $(wildcard %.hpp)
	mkdir.exe -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(CXXOPTFLAGS) -c -o $@ $<

$(OBJDIR)rsrc/%.o: %.rc
	mkdir.exe -p $(dir $@)
	$(WINDRES) $(WINDRESFLAGS) -o $@ $<

bassopenmpt.dll: $(BASS_OPENMPT_SRCS)
	$(GCC) -w -s -O3 $^ -shared -o $@ -Wl,--add-stdcall-alias -L. -lbass -lopenmpt

bass_sndfile.dll: $(BASS_SNDFILE_SRCS)
	$(GCC) -w -s -O3 $^ -shared -o $@ -Wl,--add-stdcall-alias -L. -lbass -lsndfile

bass_gme.dll: $(BASS_GME_SRCS)
	$(GCC) -w -s -O3 $^ -shared -o $@ -Wl,--add-stdcall-alias -L. -lbass -lgme.dll

bass_hvl.dll: $(BASS_HVL_SRCS)
	$(GCC) -w -s -O3  $^ -shared -o $@ -Wl,--add-stdcall-alias -L. -lbass

bass_mdxmini.dll: $(BASS_MDX_SRCS)
	$(GCC) -w -s -O3 $^ -shared -o $@ -Wl,--add-stdcall-alias -L. -lbass -lmdxmini

bassvstimidi.dll: $(BASS_VSTIMIDI_SRCS)
	$(CXX) -w -s -O3 $^ -shared -o $@ -Wl,--add-stdcall-alias -L. -lbass -lbass_vst

bass_opl.dll: $(BASS_ADPLUG_SRCS)
	$(CXX) -w -s -O3 -fpermissive $^ -Isrc/bass_opl -shared -o $@ -Wl,--add-stdcall-alias -L. -lbass -ladplug

bass_ym.dll: $(BASS_YM_SRCS)
	$(GCC) -w -s -O3 $^ -shared -o $@ -Wl,--add-stdcall-alias -L. -lbass -lSTSound -lstdc++
