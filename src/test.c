// Forward declaration
struct Point;

// Struct definition
struct Point {
    int x;
    int y;
};

// Union
union Data {
    int i;
    float f;
    char c;
};

// Enum
enum Color {
    RED,
    GREEN = 5,
    BLUE
};

// Typedef
typedef int MyInt;
typedef struct Point Point;

int main() {
    struct Point p;
    p.x = 10;
    p.y = 20;
    
    union Data d;
    d.i = 42;
    
    enum Color c = RED;
    c = GREEN;
    
    MyInt x = 100;
    
    return p.x + p.y + d.i + c + x;
}