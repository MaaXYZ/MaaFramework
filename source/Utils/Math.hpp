#pragma once

struct CubicSpline
{
    double a = 0;
    double b = 0;
    double c = 0;

    static CubicSpline smooth_in_out(double in, double out)
    {
        return CubicSpline { .a = in, .b = -(2 * in + out - 3), .c = -(-in - out + 2) };
    }

    double operator()(double t)
    {
        double t2 = t * t;
        double t3 = t2 * t;
        return a * t + b * t2 + c * t3;
    }
};
