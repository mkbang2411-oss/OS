#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <memory>
#include "Scheduler.h"
#include "fileHandler.h"

using namespace std;

bool Scheduler::isAllDone() const // Check remaining time cua tung Process
{
    for (auto &p : processes)
    {
        if (p.remainingTime > 0)
        {
            return false;
        }
    }

    return true;
}

void Scheduler::addProcessIntoQueue(int currentTime)
{
    for (int i = 0; i < processes.size(); i++)
    {
        Process &p = processes[i];

        if (!p.isInQueue && p.arrivalTime <= currentTime && p.remainingTime > 0)
        {
            queues[p.queueIndex].readyQueue.push_back(i);
            p.isInQueue = true;
        }
    }
}

SimulationResult Scheduler::run()
{
    int currentTime = 0;
    int currentQueue = 0;

    while (!isAllDone())
    {
        addProcessIntoQueue(currentTime);

        bool execute = false;

        for (int count = 0; count < queues.size(); count++)
        {
            int i = (currentQueue + count) % queues.size();

            if (!queues[i].readyQueue.empty())
            {
                queues[i].strategy->execute(queues[i], processes, currentTime, timeline, queues[i].timeSlice);
                currentQueue = (i + 1) % queues.size();
                execute = true;

                addProcessIntoQueue(currentTime);

                break;
            }
        }

        if (!execute)
        {
            currentTime++;
            addProcessIntoQueue(currentTime);
        }
    }

    calculateMetrics();

    SimulationResult result;

    double totalTurnaround = 0;
    double totalWaiting = 0;

    for (const auto &p : processes)
    {
        totalTurnaround += p.turnaroundTime;
        totalWaiting += p.waitingTime;
    }

    result.processes = processes;
    result.timeline = timeline;
    result.avgTurnaround = totalTurnaround / processes.size();
    result.avgWaiting = totalWaiting / processes.size();

    return result;
}