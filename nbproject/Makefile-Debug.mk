#
# Generated Makefile - do not edit!
#
# Edit the Makefile in the project folder instead (../Makefile). Each target
# has a -pre and a -post target defined where you can add customized code.
#
# This makefile implements configuration specific macros and targets.


# Environment
MKDIR=mkdir
CP=cp
GREP=grep
NM=nm
CCADMIN=CCadmin
RANLIB=ranlib
CC=gcc
CCC=g++
CXX=g++
FC=gfortran
AS=as

# Macros
CND_PLATFORM=GNU-Linux-x86
CND_DLIB_EXT=so
CND_CONF=Debug
CND_DISTDIR=dist
CND_BUILDDIR=build

# Include project Makefile
include Makefile

# Object Directory
OBJECTDIR=${CND_BUILDDIR}/${CND_CONF}/${CND_PLATFORM}

# Object Files
OBJECTFILES= \
	${OBJECTDIR}/sources/DependenceGraph.o \
	${OBJECTDIR}/sources/Rule.o \
	${OBJECTDIR}/sources/Utils.o \
	${OBJECTDIR}/sources/Vocabulary.o \
	${OBJECTDIR}/sources/global.o \
	${OBJECTDIR}/sources/lex.o \
	${OBJECTDIR}/sources/main.o \
	${OBJECTDIR}/sources/parse.o


# C Compiler Flags
CFLAGS=

# CC Compiler Flags
CCFLAGS=
CXXFLAGS=

# Fortran Compiler Flags
FFLAGS=

# Assembler Flags
ASFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/splittingapplication

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/splittingapplication: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}
	${LINK.cc} -o ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/splittingapplication ${OBJECTFILES} ${LDLIBSOPTIONS}

${OBJECTDIR}/sources/DependenceGraph.o: sources/DependenceGraph.cpp 
	${MKDIR} -p ${OBJECTDIR}/sources
	${RM} $@.d
	$(COMPILE.cc) -g -Iheadfiles -Iheadfiles -MMD -MP -MF $@.d -o ${OBJECTDIR}/sources/DependenceGraph.o sources/DependenceGraph.cpp

${OBJECTDIR}/sources/Rule.o: sources/Rule.cpp 
	${MKDIR} -p ${OBJECTDIR}/sources
	${RM} $@.d
	$(COMPILE.cc) -g -Iheadfiles -Iheadfiles -MMD -MP -MF $@.d -o ${OBJECTDIR}/sources/Rule.o sources/Rule.cpp

${OBJECTDIR}/sources/Utils.o: sources/Utils.cpp 
	${MKDIR} -p ${OBJECTDIR}/sources
	${RM} $@.d
	$(COMPILE.cc) -g -Iheadfiles -Iheadfiles -MMD -MP -MF $@.d -o ${OBJECTDIR}/sources/Utils.o sources/Utils.cpp

${OBJECTDIR}/sources/Vocabulary.o: sources/Vocabulary.cpp 
	${MKDIR} -p ${OBJECTDIR}/sources
	${RM} $@.d
	$(COMPILE.cc) -g -Iheadfiles -Iheadfiles -MMD -MP -MF $@.d -o ${OBJECTDIR}/sources/Vocabulary.o sources/Vocabulary.cpp

${OBJECTDIR}/sources/global.o: sources/global.cpp 
	${MKDIR} -p ${OBJECTDIR}/sources
	${RM} $@.d
	$(COMPILE.cc) -g -Iheadfiles -Iheadfiles -MMD -MP -MF $@.d -o ${OBJECTDIR}/sources/global.o sources/global.cpp

${OBJECTDIR}/sources/lex.o: sources/lex.cpp 
	${MKDIR} -p ${OBJECTDIR}/sources
	${RM} $@.d
	$(COMPILE.cc) -g -Iheadfiles -Iheadfiles -MMD -MP -MF $@.d -o ${OBJECTDIR}/sources/lex.o sources/lex.cpp

${OBJECTDIR}/sources/main.o: sources/main.cpp 
	${MKDIR} -p ${OBJECTDIR}/sources
	${RM} $@.d
	$(COMPILE.cc) -g -Iheadfiles -Iheadfiles -MMD -MP -MF $@.d -o ${OBJECTDIR}/sources/main.o sources/main.cpp

${OBJECTDIR}/sources/parse.o: sources/parse.cpp 
	${MKDIR} -p ${OBJECTDIR}/sources
	${RM} $@.d
	$(COMPILE.cc) -g -Iheadfiles -Iheadfiles -MMD -MP -MF $@.d -o ${OBJECTDIR}/sources/parse.o sources/parse.cpp

# Subprojects
.build-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${CND_BUILDDIR}/${CND_CONF}
	${RM} ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/splittingapplication

# Subprojects
.clean-subprojects:

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
