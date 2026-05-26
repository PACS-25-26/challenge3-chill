#include "functions.hpp"
#include <cmath>
#include <numbers>

constexpr double PI = std::numbers::pi;

double f(double x, double y) {
    return 8.0 * PI * PI * std::sin(2.0 * PI * x) * std::sin(2.0 * PI * y);
}

double exact(double x, double y) {
    return std::sin(2.0 * PI * x) * std::sin(2.0 * PI * y);
}