int x = 5;
int x = 10;  // ✗ Redefinition of 'x'

void foo() {
    y = 5;     // ✗ Use of undeclared identifier 'y'
}

int bar(int a, int a) {  // ✗ Duplicate parameter name 'a'
    return a;
}

int baz() {
    return baz;  // ✓ Functions are symbols too
}

int baz() {      // ✗ Redefinition of function 'baz'
    return 1;
}

int baz();       // ✓ Forward declaration is OK
int baz();       // ✓ Multiple declarations OK