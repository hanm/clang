// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker %s -verify
//
// expected-no-diagnostics


float circumference(long radius) {
	return 2* 3.14 * radius;
}

int floatToInt(float x) { return x; }
long doubleToLong(double x) { return x; }
long intToLong(int x) { return x; }
float doubleToFloat(double x) { return x; }
bool intToBool(int x) { return x; }
bool floatToBool(int x) { return x; }

_Complex int intToComplexInt(int x) { return x; }
_Complex long intToComplexLong(int x) { return x; }
_Complex double longToComplexDouble(long x) { return x; }
_Complex short doubleToComplexShort(float x) { return x; }
_Complex float floatToComplexFloat(float x) { return x; }
_Complex float doubleToComplexFloat(double x) { return x; }

_Complex long complexDoubleToComplexLong(_Complex double x) { return x; }
_Complex float complexLongToComplexFloat(_Complex long x) { return x; }
_Complex double complexFloatToComplexDouble(_Complex float x) { return x; }
_Complex long long complexLongToComplexLongLong(_Complex long x) { return x; }


bool complexFloatToBool(_Complex float x) { return x; }
bool complexCharToBool(_Complex char x) { return x; }
double complexFloatToDouble(_Complex float x) { return x; }
long complexDoubleToLong(_Complex double x) { return x; }
long complexIntToLong(_Complex int x) { return x; }
float complexLongToFloat(_Complex long x) { return x; }

unsigned int signedIntToUnsignedInt(int x) { return x; }

bool pointerToBool(void *x) { return x; }

