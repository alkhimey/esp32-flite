#
# Component Makefile
#

# Source files from flite that are not supposed to be compiled on 
EXCLUDE_SRCS:=src/audio/au_alsa.c \
             src/audio/au_wince.c \
             src/audio/au_win.c \
             src/audio/au_pulseaudio.c \
             src/audio/au_sun.c \
             src/audio/au_palmos.c \
             src/audio/au_oss.c \
             src/utils/cst_file_wince.c\
             src/utils/cst_mmap_posix.c\
             src/utils/cst_mmap_win32.c\
             src/utils/cst_file_palmos.c

COMPONENT_SRCS := $(filter-out $(EXCLUDE_SRCS), $(wildcard $(src)/**/*.c))
COMPONENT_OBJS := $(patsubst %.c, %.o, $(COMPONENT_SRCS))

COMPONENT_SRCDIRS := src/audio src/cg src/htg src/lexicon src/regex src/speech src/stats src/synth src/utils src/wavesynth


# COMPONENT_ADD_INCLUDEDIRS : Paths, relative to the component directory, which will be added to the include search path for all components in the project. Defaults to include if not overridden. If an include directory is only needed to compile this specific component, add it to COMPONENT_PRIV_INCLUDEDIRS instead.


# COMPONENT_DEPENDS: Optional list of component names that should be compiled before this component. This is not necessary for link-time dependencies, because all component include directories are available at all times. It is necessary if one component generates an include file which you then want to include in another component. Most components do not need to set this variable.

# COMPONENT_PRIV_INCLUDEDIRS: Directory paths, must be relative to the component directory, which will be added to the include search path for this component’s source files only.

# Code and Data Placements
#
# eSP-IDF has a feature called linker script generation that enables components to define where its code and data will be placed in memory through linker fragment files. These files are processed by the build system, and is used to augment the linker script used for linking app binary. See Linker Script Generation for a quick start guide as well as a detailed discussion of the mechanism.

