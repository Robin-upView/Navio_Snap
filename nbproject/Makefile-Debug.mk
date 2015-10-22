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
	${OBJECTDIR}/I2Cdev.o \
	${OBJECTDIR}/MPU9250.o \
	${OBJECTDIR}/PCA9685.o \
	${OBJECTDIR}/gpio.o \
	${OBJECTDIR}/main.o


# C Compiler Flags
CFLAGS=

# CC Compiler Flags
CCFLAGS=-lpigpio -lrt -lpthread
CXXFLAGS=-lpigpio -lrt -lpthread

# Fortran Compiler Flags
FFLAGS=

# Assembler Flags
ASFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=../control_law_snap/dist/Debug/GNU-Linux-x86/libcontrol_law_snap.a

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/snap

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/snap: ../control_law_snap/dist/Debug/GNU-Linux-x86/libcontrol_law_snap.a

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/snap: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}
	${LINK.cc} -o ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/snap ${OBJECTFILES} ${LDLIBSOPTIONS} -lpigpio -lrt -lpthread

${OBJECTDIR}/I2Cdev.o: nbproject/Makefile-${CND_CONF}.mk I2Cdev.cpp 
	${MKDIR} -p ${OBJECTDIR}
	$(COMPILE.cc) -g -I../control_law_snap -o ${OBJECTDIR}/I2Cdev.o I2Cdev.cpp

${OBJECTDIR}/MPU9250.o: nbproject/Makefile-${CND_CONF}.mk MPU9250.cpp 
	${MKDIR} -p ${OBJECTDIR}
	$(COMPILE.cc) -g -I../control_law_snap -o ${OBJECTDIR}/MPU9250.o MPU9250.cpp

${OBJECTDIR}/PCA9685.o: nbproject/Makefile-${CND_CONF}.mk PCA9685.cpp 
	${MKDIR} -p ${OBJECTDIR}
	$(COMPILE.cc) -g -I../control_law_snap -o ${OBJECTDIR}/PCA9685.o PCA9685.cpp

${OBJECTDIR}/gpio.o: nbproject/Makefile-${CND_CONF}.mk gpio.cpp 
	${MKDIR} -p ${OBJECTDIR}
	$(COMPILE.cc) -g -I../control_law_snap -o ${OBJECTDIR}/gpio.o gpio.cpp

${OBJECTDIR}/main.o: nbproject/Makefile-${CND_CONF}.mk main.cpp 
	${MKDIR} -p ${OBJECTDIR}
	$(COMPILE.cc) -g -I../control_law_snap -o ${OBJECTDIR}/main.o main.cpp

# Subprojects
.build-subprojects:
	cd ../control_law_snap && ${MAKE}  -f Makefile CONF=Debug

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${CND_BUILDDIR}/${CND_CONF}
	${RM} ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/snap

# Subprojects
.clean-subprojects:
	cd ../control_law_snap && ${MAKE}  -f Makefile CONF=Debug clean
