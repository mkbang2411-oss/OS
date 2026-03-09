#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include "Scheduler.h"
using namespace std;

struct Block { string label; int span; };
class FileHandler // Khanh Bang
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