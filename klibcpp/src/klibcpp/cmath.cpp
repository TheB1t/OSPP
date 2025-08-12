#include <klibcpp/cmath.hpp>

int32_t abs(int32_t in) {
    if (in < 0) 
        return -in;

    return in;
}

double pow(double x, int y) {
    if (y == 0)
        return 1.0;

    if (y < 0) {
        x = 1.0 / x;
        y = -y;
    }

    double result = 1.0;
    while (y > 0) {
        if (y % 2 == 1) {
            result *= x;
        }
        x *= x;
        y /= 2;
    }

    return result;
}