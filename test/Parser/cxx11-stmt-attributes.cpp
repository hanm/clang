// RUN: %clang_cc1 -fcxx-exceptions -fexceptions -fsyntax-only -verify -std=c++11 %s

void foo(int i) {

  [[unknown_attribute]] ; // expected-warning {{attribute unknown_attribute cannot be specified on a statement}}
  [[unknown_attribute]] { } // expected-warning {{attribute unknown_attribute cannot be specified on a statement}}
  [[unknown_attribute]] if (0) { } // expected-warning {{attribute unknown_attribute cannot be specified on a statement}}
  [[unknown_attribute]] for (;;); // expected-warning {{attribute unknown_attribute cannot be specified on a statement}}
  [[unknown_attribute]] do { // expected-warning {{attribute unknown_attribute cannot be specified on a statement}}
    [[unknown_attribute]] continue; // expected-warning {{attribute unknown_attribute cannot be specified on a statement}}
  } while (0);
  [[unknown_attribute]] while (0); // expected-warning {{attribute unknown_attribute cannot be specified on a statement}}

  [[unknown_attribute]] switch (i) { // expected-warning {{attribute unknown_attribute cannot be specified on a statement}}
    [[unknown_attribute]] case 0: // expected-warning {{attribute unknown_attribute cannot be specified on a statement}}
    [[unknown_attribute]] default: // expected-warning {{attribute unknown_attribute cannot be specified on a statement}}
      [[unknown_attribute]] break; // expected-warning {{attribute unknown_attribute cannot be specified on a statement}}
  }

  [[unknown_attribute]] goto here; // expected-warning {{attribute unknown_attribute cannot be specified on a statement}}
  [[unknown_attribute]] here: // expected-warning {{unknown attribute 'unknown_attribute' ignored}}

  [[unknown_attribute]] try { // expected-warning {{attribute unknown_attribute cannot be specified on a statement}}
  } catch (...) {
  }

  [[unknown_attribute]] return; // expected-warning {{attribute unknown_attribute cannot be specified on a statement}}
	 

  alignas(8) ; // expected-warning {{attribute aligned cannot be specified on a statement}}
  [[noreturn]] { } // expected-warning {{attribute noreturn cannot be specified on a statement}}
  [[noreturn]] if (0) { } // expected-warning {{attribute noreturn cannot be specified on a statement}}
  [[noreturn]] for (;;); // expected-warning {{attribute noreturn cannot be specified on a statement}}
  [[noreturn]] do { // expected-warning {{attribute noreturn cannot be specified on a statement}}
    [[unavailable]] continue; // expected-warning {{attribute unavailable cannot be specified on a statement}}
  } while (0);
  [[unknown_attributqqq]] while (0); // expected-warning {{attribute unknown_attributqqq cannot be specified on a statement}}
	// TODO: remove 'qqq' part and enjoy 'empty loop body' warning here (DiagnoseEmptyLoopBody)

  [[unknown_attribute]] while (0); // expected-warning {{attribute unknown_attribute cannot be specified on a statement}}

  [[unused]] switch (i) { // expected-warning {{attribute unused cannot be specified on a statement}}
    [[uuid]] case 0: // expected-warning {{attribute uuid cannot be specified on a statement}}
    [[visibility]] default: // expected-warning {{attribute visibility cannot be specified on a statement}}
      [[carries_dependency]] break; // expected-warning {{attribute carries_dependency cannot be specified on a statement}}
  }

  [[fastcall]] goto there; // expected-warning {{attribute fastcall cannot be specified on a statement}}
  [[noinline]] there: // expected-warning {{unknown attribute 'noinline' ignored}}

  [[lock_returned]] try { // expected-warning {{attribute lock_returned cannot be specified on a statement}}
  } catch (...) {
  }

  [[weakref]] return; // expected-warning {{attribute weakref cannot be specified on a statement}}
}
