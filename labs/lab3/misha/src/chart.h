#pragma once
#include <vector>
#include <string>
#include <functional>
#include <thread>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>

class Chart {
private:
    void Clear() {
            CurrentStep = 0;

            Offsets.resize(NumberOfParticles, 0.0);
            Offsets[NumberOfParticles / 2 - 1] = InitialDeviation;
            Offsets[NumberOfParticles / 2] = -InitialDeviation;

            Speeds.resize(NumberOfParticles, 0.0);
            Accelerations = CalcCommonAccelerations();
        }
protected:
    std::string Header;
    bool IsStarted = false;
    double Alpha = 0.0;
    double Beta = 0.7;
    double Tau = 0.02;
    double Mass = 1;
    int NumberOfSteps = 1'000'00;
    int CurrentStep = 0;
    int UpdateStep = 50;
    int NumberOfParticles = 500;
    double InitialDeviation = 1;
    double InitalHamiltonian = 0.0;
    double FiniteHamiltonian = 0.0;

    std::vector<double> Offsets;
    std::vector<double> Speeds;
    std::vector<double> Accelerations;

    std::ofstream fout;

public:
    Chart(const std::string& header) : Header(header) {
        Clear();
        fout.open("../labs/lab3/misha/result/" + header + ".txt");
    }

    void SetAlphaBetta(double Alpha,double Beta){
        this -> Alpha = Alpha;
        this -> Beta = Beta;
        InitalHamiltonian = CalcHamiltonian();
        std :: cout << "Начальный гамильтолиан для "<< Header << ": "<< std::fixed << std::setprecision(15) << InitalHamiltonian  << std::endl;
    }

    ~Chart() {
        fout.close();
        std :: cout << "Конечный гамильтолиан для " << Header << ": " << std::fixed << std::setprecision(15) << FiniteHamiltonian << std::endl;
    }

protected:
    
    void SaveResult(){
        for (auto value : Offsets ){
            fout << value << " "; 
        }
        fout << std::endl;
    }

    std::vector<double> CalcCommonAccelerations() {
        auto gradV = CalcGradV();
        for (auto& acc : gradV) {
            acc *= -1.0 / Mass;
        }
        return gradV;
    }

    double CalcHamiltonian() {
        double result = 0.0;
        auto V = CalcV();
        for (size_t i = 0; i < V.size(); ++i) {
            result += Speeds[i] * Speeds[i] * Mass / 2 + V[i];
        }
        return result;
    }

    std::vector<double> CalcGradV() {
        std::vector<double> result(NumberOfParticles);
        
        result[0] = CalcGradV(Offsets[NumberOfParticles - 1], Offsets[0], Offsets[1]);
        result[NumberOfParticles - 1] = CalcGradV(Offsets[NumberOfParticles - 2], 
                                                  Offsets[NumberOfParticles - 1], 
                                                  Offsets[0]);

        for (int i = 1; i < NumberOfParticles - 1; ++i) {
            result[i] = CalcGradV(Offsets[i - 1], Offsets[i], Offsets[i + 1]);
        }

        return result;
    }

    double CalcGradV(double qM1, double q, double qP1) {
        return -(qP1 - q) + (q - qM1) +
               Alpha * (-std::pow(qP1 - q, 2) + std::pow(q - qM1, 2)) +
               Beta * (-std::pow(qP1 - q, 3) + std::pow(q - qM1, 3));
    }

    std::vector<double> CalcV() {
        std::vector<double> result(Offsets.size());
        for (size_t i = 0; i < result.size() - 1; ++i) {
            result[i] = CalcV(Offsets[i], Offsets[i + 1]);
        }
        result[result.size() - 1] = CalcV(Offsets[result.size() - 1], Offsets[0]);
        return result;
    }

    double CalcV(double q, double qP1) {
        double r = qP1 - q;
        return r * r / 2.0 + Alpha * r * r * r / 3.0 + Beta * r * r * r * r / 4.0;
    }
};