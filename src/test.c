// Test file for C23 Lexer
// This tests all token types

/* Multi-line
   comment test */

#include <stdio.h>
#define MAX 100
#define ADD(a, b) ((a) + (b))

// C23 Keywords
constexpr int factorial(int n) {
    return n <= 1 ? 1 : n * factorial(n - 1);
}

static_assert(factorial(5) == 120, "Factorial test failed");

// Number literals
int decimal = 42;
int octal = 077;
int hex = 0xFF;
int binary = 0b1010;
int with_separator = 1'000'000;
int hex_sep = 0xFF'FF;

// Float literals
double pi = 3.14;
double scientific = 1.5e10;
double hex_float = 0x1.5p3;
float small = 1.5f;
long double big = 3.14L;

// String literals
const char* simple = "hello";
const char* with_escape = "hello\nworld\t!";
const char* adjacent = "me" "ow";
const char* quotes = "He said \"hi\"";
const char* backslash = "path\\to\\file";

// Character literals
char letter = 'a';
char newline = '\n';
char backslash_char = '\\';
char quote = '\'';

// All operators
int a = 5;
int b = 10;
a++;
b--;
a += 5;
b -= 3;
a *= 2;
b /= 4;
a %= 3;
int c = a & b;
int d = a | b;
int e = a ^ b;
int f = ~a;
int g = !a;
int h = a << 2;
int i = b >> 1;
a <<= 2;
b >>= 1;
bool j = (a == b);
bool k = (a != b);
bool l = (a < b);
bool m = (a > b);
bool n = (a <= b);
bool o = (a >= b);
bool p = (a && b);
bool q = (a || b);

// Pointer and member access
struct Point { int x; int y; };
struct Point p1;
struct Point* ptr = &p1;
ptr->x = 5;
p1.y = 10;

// Variadic macro test
#define LOG(fmt, ...) printf(fmt, __VA_ARGS__)

// C23 Attributes
[[noreturn]] void exit_program() {
    while(1) {}
}

[[nodiscard]] int important_function() {
    return 42;
}

[[deprecated("Use new_func instead")]] void old_func() {}

// Control flow
if (a > b) {
    return a;
} else {
    return b;
}

switch(a) {
    case 1:
        break;
    case 2:
        [[fallthrough]];
    default:
        break;
}

while (a < 10) {
    a++;
}

for (int i = 0; i < 10; i++) {
    continue;
}

do {
    b--;
} while (b > 0);

// Ellipsis (variadic function)
void variadic_func(int count, ...) {
    // ...
}

// typeof (C23)
typeof(a) another_a = a;
typeof_unqual(const int) unqual_var = 5;

// nullptr (C23)
int* null_ptr = nullptr;

// bool, true, false (C23)
bool flag = true;
bool other = false;

// Preprocessor with various directives
#ifdef DEBUG
    #warning "Debug mode enabled"
#elif defined(RELEASE)
    #pragma optimize("speed")
#else
    #error "Unknown build type"
#endif

#ifndef MAX_SIZE
    #define MAX_SIZE 1000
#endif

#line 100 "custom_file.c"

// Token pasting and stringify (in macro context)
#define STRINGIFY(x) #x
#define PASTE(a, b) a##b

// Edge cases
int underscore_var = 5;
int _private = 10;
int var123 = 20;

// Empty statements
;
;

// Complex expressions
int complex = (a + b) * (c - d) / (e % f);
int ternary = (a > b) ? a : b;

// Comma operator
int x = (a = 5, b = 10, a + b);

// End of file test