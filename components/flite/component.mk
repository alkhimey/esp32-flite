#
# Component Makefile
#

COMPONENT_SRCDIRS := src/audio src/cg src/hrg src/lexicon src/regex src/speech src/stats src/synth src/utils src/wavesynth


# COMPONENT_ADD_INCLUDEDIRS : Paths, relative to the component directory, which will be added to the include search path for all components in the project. Defaults to include if not overridden. If an include directory is only needed to compile this specific component, add it to COMPONENT_PRIV_INCLUDEDIRS instead.


# COMPONENT_DEPENDS: Optional list of component names that should be compiled before this component. This is not necessary for link-time dependencies, because all component include directories are available at all times. It is necessary if one component generates an include file which you then want to include in another component. Most components do not need to set this variable.

# COMPONENT_PRIV_INCLUDEDIRS: Directory paths, must be relative to the component directory, which will be added to the include search path for this component’s source files only.

# Code and Data Placements
#
# eSP-IDF has a feature called linker script generation that enables components to define where its code and data will be placed in memory through linker fragment files. These files are processed by the build system, and is used to augment the linker script used for linking app binary. See Linker Script Generation for a quick start guide as well as a detailed discussion of the mechanism.

