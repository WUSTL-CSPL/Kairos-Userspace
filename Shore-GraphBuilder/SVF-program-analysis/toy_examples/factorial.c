#include <stdio.h>


int main(int argc, char **argv) {
    int n, i;
    printf("Hello world\n");
    int fact = 1;
    printf("Hello world\n");
    printf("Enter an integer: ");
    scanf("%d", &n);

    // shows error if the user enters a negative integer
    if (n < 0)
        printf("Error! Factorial of a negative number doesn't exist.");
    else {
        for (i = 1; i <= n; ++i) {
            fact *= i;
        }
        printf("Factorial of %d = %d \n", n, fact);
    }

    return 0;
}
