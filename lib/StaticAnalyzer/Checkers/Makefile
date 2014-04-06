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
LIBRARYNAME := clangStaticAnalyzerCheckers

BUILT_SOURCES = Checkers.inc
TABLEGEN_INC_FILES_COMMON = 1

include $(CLANG_LEVEL)/Makefile

# Prolog Libraries (the LDFLAGS don't seem to take effect when changed here,
# whether these lines are before or after the 'include' of the main Makefile)
# PROLOG_LIB_INCLUDE='/usr/lib/swi-prolog/include'
#PROLOG_LIB_LINK='/usr/lib'
# CPP.Flags += -I$(PROLOG_LIB_INCLUDE)
#LDFLAGS += -L$(PROLOG_LIB_LINK) -lswipl

$(ObjDir)/Checkers.inc.tmp : Checkers.td $(PROJ_SRC_DIR)/$(CLANG_LEVEL)/include/clang/StaticAnalyzer/Checkers/CheckerBase.td $(CLANG_TBLGEN) $(ObjDir)/.dir
	$(Echo) "Building Clang SA Checkers tables with tblgen"
	$(Verb) $(ClangTableGen) -gen-clang-sa-checkers -I $(PROJ_SRC_DIR)/$(CLANG_LEVEL)/include -o $(call SYSPATH, $@) $<
