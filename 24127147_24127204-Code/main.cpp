#include <iostream>
#include "Scheduler.h"
#include "fileHandler.h"

using namespace std;

int main(int argc, char* argv[])
{
    if (argc < 3)
    {
        cerr << "Usage: " << argv[0] << " <input_file> <output_file>\n";
        return 1;
    }

    vector<CPUQueue> queues;
    vector<Process>  processes;

    FileHandler::readInput(argv[1], queues, processes);

    Scheduler scheduler(move(queues), move(processes));

    SimulationResult result = scheduler.run();

    FileHandler::writeOutput(argv[2], result);

    cout <<endl<< "Done! " << "\n";

    return 0;
}