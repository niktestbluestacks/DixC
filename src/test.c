int main() {
    int x = 5;
    
    // Valid uses
    if (x > 0) [[likely]] {
        return 1;
    }
    
    if (x < 0) [[unlikely]] {
        return -1;
    } 
    else [[likely]]{
        return 12;
    }
    
    switch (x) {
        case [[likely]] 1:
            return 10;
        case 2: [[unlikely]]
            return 20;
        default: [[unlikely]]
            return 0;
    }
    
    // Invalid uses
    [[likely]] int y = 10;  // ERROR: likely on variable
    
    return 0;
}