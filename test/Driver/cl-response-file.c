// Don't attempt slash switches on msys bash.
// REQUIRES: shell-preserves-root

// Test that we use the Windows tokenizer for clang-cl response files. The
// trailing backslash before the space should be interpreted as a literal
// backslash. PR23709



// RUN: echo '/I%S\Inputs\cl-response-file\ /DFOO=2' > %t.rsp
// RUN: %clang_cl /c -### @%t.rsp -- %s 2>&1 | FileCheck %s

// CHECK: "-D" "FOO=2" "-I" "{{.*}}\\Inputs\\cl-response-file\\"
