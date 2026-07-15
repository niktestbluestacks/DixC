int factorial(int n) {
    if (n <= 1) {
        return 1;
    }
    return n * factorial(n - 1);
}

int main() {
    int x = 5;
    int result = factorial(x);
    
    int arr[10];
    arr[0] = 42;
    arr[1] = arr[0] + 1;
    
    int i = 0;
    while (i < 10) {
        arr[i] = i * i;
        i++;
    }
    int j;
    for (j = 0; j < 5; j++) {
        if (arr[j] > 10) {
            break;
        }
    }

    for (const int restrict * j = 0; ;) {
        break;
    }
    
    int y = x > 0 ? x : -x;
    
    return 0;
}