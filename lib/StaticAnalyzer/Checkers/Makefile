##===- clang/lib/Checker/Makefile --------------------------*- Makefile -*-===##
# 
#                     The LLVM Compiler Infrastructure
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
# 
##===----------------------------------------------------------------------===##
#
# This implements analyses built on top of source-level CFGs. 
#
##===----------------------------------------------------------------------===##

CLANG_LEVEL := ../../..

# BASE_SOURCES: Source files of Clang checkers. We use PROJ_SRC_DIR because the
#   makefile will be invoked in the build tree (where the source isn't).
BASE_SOURCES = $(notdir $(wildcard $(PROJ_SRC_DIR)/*.cpp)) 
# ASAP_DIR: Directory where most of the ASaP code lives
ASAP_DIR = ASaP
# ASAP_NAMES: The names of the .cpp files under ASAP_DIR
ASAP_NAMES = $(notdir $(wildcard $(PROJ_SRC_DIR)/$(ASAP_DIR)/*.cpp))
# ASAP_SOURCES: Computed by prepending ASAP_DIR to ASAP_NAMES
ASAP_SOURCES = $(ASAP_NAMES:%=$(ASAP_DIR)/%)
# SOURCES: Override the default to add the ASAP_SOURCES
SOURCES = $(BASE_SOURCES) $(ASAP_SOURCES)

LIBRARYNAME := clangStaticAnalyzerCheckers

BUILT_SOURCES = Checkers.inc
TABLEGEN_INC_FILES_COMMON = 1

include $(CLANG_LEVEL)/Makefile

# Prolog Libraries (the LDFLAGS don't seem to take effect when changed here,
# whether these lines are before or after the 'include' of the main Makefile)
PROLOG_LIB_INCLUDE='/usr/lib/swi-prolog/include'
#PROLOG_LIB_LINK='/usr/lib'
CPP.Flags += -I$(PROLOG_LIB_INCLUDE)
#LDFLAGS += -L$(PROLOG_LIB_LINK) -lswipl

$(ObjDir)/Checkers.inc.tmp : Checkers.td $(PROJ_SRC_DIR)/$(CLANG_LEVEL)/include/clang/StaticAnalyzer/Checkers/CheckerBase.td $(CLANG_TBLGEN) $(ObjDir)/.dir
	$(Echo) "Building Clang SA Checkers tables with tblgen"
	$(Verb) $(ClangTableGen) -gen-clang-sa-checkers -I $(PROJ_SRC_DIR)/$(CLANG_LEVEL)/include -o $(call SYSPATH, $@) $<
