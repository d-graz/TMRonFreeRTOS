#include <stdio.h>

double calculatePi(int decimalPlaces) {
    double pi = 0.0;
    int sign = 1;
    int denominator = 1;

    for (int i = 0; i < decimalPlaces; i++) {
        pi += (double) sign / denominator;
        sign *= -1;
        denominator += 2;
    }

    return pi * 4;
}

/*int main() {
    int decimalPlaces;
    printf("Enter the number of decimal places for pi: ");
    scanf("%d", &decimalPlaces);

    double pi = calculatePi(decimalPlaces);
    printf("Pi: %.15f\n", pi);

    return 0;
}*/
