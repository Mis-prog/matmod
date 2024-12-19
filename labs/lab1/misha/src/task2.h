#pragma once

#include <iostream>
#include <cmath>
#include <boost/numeric/odeint.hpp>
#include <fstream>
#include <chrono>
#include <boost/math/tools/minima.hpp>

using namespace boost::numeric::odeint;
using state_type = std::array<double, 8>;
using namespace std;


// Физические константы
struct Constants {
    static constexpr double G = 6.67e-11; // гравитационная постоянная
    static constexpr double M1 = 2.0e30; // масса звезды (кг)
    static constexpr double M2 = 6.4e23; // масса планеты (кг)
    static constexpr double M3 = 1.1e16; // масса спутника (кг)
    static constexpr double R1 = 696340e3; // радиус звезды (м)
    static constexpr double R2 = 3390e3; // радиус планеты (м)
    static constexpr double R3 = 11.1e3; // радиус спутника (м)
    static constexpr double R12 = 228e9; // начальное расстояние звезда-планета (м)
    static constexpr double R23 = 9.4e6; // начальное расстояние планета-спутника (м)
    static constexpr double U2 = 24e3; // начальная скорость планеты (м/с)
    static constexpr double U3 = 2.14e3; // начальная скорость спутника (м/с)

    static constexpr double T = 1200.0; // время работы двигателя (с)
    static constexpr double H = 200e3; // высота орбиты (м)
    static constexpr double M0 = 10.0; // масса полезной назрузки (кг)
    static constexpr double U = 3040.0; // скорость истечения (м/c)
    static constexpr double koef = 0.05;

    static constexpr double MIN_ANGLE = 0.0;      // минимальный угол (радианы)
    static constexpr double MAX_ANGLE = 2 * M_PI; // максимальный угол
    static constexpr double MIN_FUEL = 50.0;      // минимальное количество топлива (кг)
    static constexpr double MAX_FUEL = 150.0;     // максимальное количество топлива
};

// Вспомогательные функции
class Physics {
public:
    static double mt;
    static double r12x; // координаты планеты
    static double r12y;
    static bool trajectory_crossed;
    static double prev_distance;
    static double min_distance;      // минимальное достигнутое расстояние до спутника
    static double final_distance;


    static double distance(double x1, double y1, double x2, double y2) {
        double dx = x2 - x1;
        double dy = y2 - y1;
        return std::sqrt(dx * dx + dy * dy);
    }

    static double m(double t) {
        if (t >= Constants::T) {
            return Constants::M0;
        } else {
            return (Constants::M0 + mt) / (1 - Constants::koef) - (mt * t) / Constants::T;
        }
    }

    static double dm(double t) {
        if (t > Constants::T) {
            return 0.0;
        } else {
            return -mt / Constants::T;
        }
    }

    static void calculateForces(const state_type &y, state_type &f, double t) {
        // Координаты и скорости спутника и ракеты
        double rx = y[0], ry = y[1], vx = y[4], vy = y[5]; // ракета
        double r13x = y[2], r13y = y[3], v3x = y[6], v3y = y[7]; // cпутник


        double current_distance = distance(rx, ry, r13x, r13y);
        min_distance = std::min(min_distance, current_distance);

        if (!trajectory_crossed && prev_distance != 0) {
            if (current_distance < Constants::R3 * 10) {
                trajectory_crossed = true;
                final_distance = current_distance;
                throw std::runtime_error("Траектория пересечена!");
            }
        }
        prev_distance = current_distance;

        // Расчёт расстояний
        double r = std::sqrt(rx * rx + ry * ry); // расстояние от солнца до ракеты
        double r2 = distance(rx, ry, r12x, r12y); // расстояние от ракеты до планеты
        double r3 = distance(rx, ry, r13x, r13y); // расстояние от ракеты до спутника
        double r13 = std::sqrt(r13x * r13x + r13y * r13y); // расстояние от спутника до солнца
        double r23 = distance(r13x, r13y, r12x, r12y); // расстояние от спутника до планеты

        // Расчет скорости
        double v = std::sqrt(vx * vx + vy * vy); // скорость ракеты


        // Скорости
        f[0] = vx;
        f[1] = vy; // ракета
        f[2] = v3x;
        f[3] = v3y; // спутник

        // Ускорения для ракеты
        f[4] = -(Constants::U * dm(t) * vx) / (v * m(t)) +
               Constants::G * (
                       -Constants::M1 * rx / std::pow(r, 3)
                       - Constants::M2 * (rx - r12x) / std::pow(r2, 3)
                       - Constants::M3 * (rx - r13x) / std::pow(r3, 3)
               );

        f[5] = -(Constants::U * dm(t) * vy) / (v * m(t)) +
               Constants::G * (
                       -Constants::M1 * ry / std::pow(r, 3)
                       - Constants::M2 * (ry - r12y) / std::pow(r2, 3)
                       - Constants::M3 * (ry - r13y) / std::pow(r3, 3)
               );

        // Ускорения для спутника
        f[6] = -Constants::G * Constants::M1 * r13x / std::pow(r13, 3) -
               Constants::G * Constants::M2 * (r13x - r12x) / std::pow(r23, 3);

        f[7] = -Constants::G * Constants::M1 * r13y / std::pow(r13, 3) -
               Constants::G * Constants::M2 * (r13y - r12y) / std::pow(r23, 3);
    }

    static double simulate_trajectory(double angle, double fuel_mass) {
        try {
            min_distance = std::numeric_limits<double>::max();
            final_distance = std::numeric_limits<double>::max();
            trajectory_crossed = false;
            prev_distance = 0;
            mt = fuel_mass;

            // Начальные условия (те же, что и в main)
            double r12x0 = -166486522781.19, r12y0 = -149495158174.03,
                    v2x0 = 16237.99, v2y0 = -18387.63,
                    r13x0 = -166476539755.17, r13y0 = -220575669506.13,
                    v3x0 = 26035.80, v3y0 = -5315.35;

            r12x = r12x0;
            r12y = r12y0;

            // Расчет начальных значений для ракеты
            double r3x = r13x0 - r12x, r3y = r13y0 - r12y;
            double r3 = std::sqrt(r3x * r3x + r3y * r3y);

            double v0 = 1.0 * std::sqrt(Constants::G * Constants::M2 / (Constants::R2 + Constants::H));
            double rx0 = (Constants::R2 + Constants::H) * (r3x * cos(angle) - r3y * sin(angle)) / r3;
            double ry0 = (Constants::R2 + Constants::H) * (r3x * sin(angle) + r3y * cos(angle)) / r3;
            double r0 = sqrt(rx0 * rx0 + ry0 * ry0);
            double vx0 = -v0 * ry0 / r0;
            double vy0 = v0 * rx0 / r0;

            rx0 += r12x;
            ry0 += r12y;

            state_type y = {
                    rx0, ry0, r13x0, r13y0,
                    vx0, vy0, v3x0 - v2x0, v3y0 - v2y0
            };

            double t = 0.0;
            double t_end = 60.0 * 60 * 24;  // 24 часа
            double h = 0.1;

            runge_kutta_dopri5<state_type> stepper;
            integrate_adaptive(stepper, calculateForces, y, t, t_end, h,
                               [](const state_type &, double) {});

        } catch (const std::runtime_error &) {
            return final_distance;
        }

        // Если траектория не пересечена, используем минимальное расстояние с штрафом
        return min_distance + 1e6;  // Штраф за непересечение
    }

};

double Physics::mt;
double Physics::r12x;
double Physics::r12y;
bool Physics::trajectory_crossed = false;
double Physics::prev_distance = 0;
double Physics::min_distance = std::numeric_limits<double>::max();
double Physics::final_distance = std::numeric_limits<double>::max();

class OptimizationProblem {
public:
    double operator()(const std::vector<double> &x) const {
        double angle = x[0];
        double fuel = x[1];

        // Проверка границ
        if (angle < Constants::MIN_ANGLE || angle > Constants::MAX_ANGLE ||
            fuel < Constants::MIN_FUEL || fuel > Constants::MAX_FUEL) {
            return std::numeric_limits<double>::max();
        }

        return Physics::simulate_trajectory(angle, fuel);
    }
};
