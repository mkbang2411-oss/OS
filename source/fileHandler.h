#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include "Scheduler.h"
using namespace std;

class FileHandler // Ngoc Minh
{
public:
    static void readInput(
        const string &filename,
        vector<CPUQueue> &queues,
        vector<Process> &processes);

    static void writeOutput(
        const string &filename,
        const SimulationResult &result);
};