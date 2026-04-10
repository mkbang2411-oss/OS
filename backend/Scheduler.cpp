#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <memory>
#include "Scheduler.h"

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

void Scheduler::addProcessIntoQueue(vector<CPUQueue> &queues, vector<Process> &processes, int currentTime)
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
        Scheduler::addProcessIntoQueue(queues, processes, currentTime);

        bool executed = false; // Check Time tro^'ng trong CPU timeSlice

        for (int count = 0; count < queues.size(); count++) // Dam bao dung thu tu RoundRobin
        {
            int i = (currentQueue + count) % queues.size();

            if (!queues[i].readyQueue.empty())
            {
                queues[i].strategy->execute(queues[i], queues, processes, currentTime, timeline, queues[i].timeSlice);
                currentQueue = (i + 1) % queues.size();
                executed = true;

                Scheduler::addProcessIntoQueue(queues, processes, currentTime);

                break;
            }
        }

        if (!executed)
        {
            currentTime++;
            Scheduler::addProcessIntoQueue(queues, processes, currentTime);
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
    if (!processes.empty())
    {
        result.avgTurnaround = totalTurnaround / processes.size();
        result.avgWaiting = totalWaiting / processes.size();
    }

    return result;
}

void SJFStrategy ::execute(CPUQueue &queue, vector<CPUQueue> &queues, vector<Process> &process, int &currentTime, vector<GanttEntry> &timeline, int timeSlice)
{
    if (queue.readyQueue.empty())
        return;

    int min = 0;
    // Tìm readyQueue có remainingTime nhỏ nhất
    for (int i = 1; i < queue.readyQueue.size(); i++)
    {
        int index1 = queue.readyQueue[i];
        int index2 = queue.readyQueue[min];
        if (process[index1].remainingTime < process[index2].remainingTime)
        {
            min = i;
        }
    }

    int indexProcess = queue.readyQueue[min];
    Process &p = process[indexProcess];
    int startTime = currentTime;

    // Check process đã chạy xong hay chưa
    if (p.remainingTime <= timeSlice)
    {
        currentTime += p.remainingTime;
        timeline.push_back({startTime, currentTime, queue.qid, p.pid});
        p.remainingTime = 0;
        p.completionTime = currentTime;
        p.isInQueue = false;
        // xóa khỏi queue khi process đã chạy xong
        queue.readyQueue.erase(queue.readyQueue.begin() + min);

        // De quy check lai neu con du thoi gian cho nhieu Queue
        int usedTime = currentTime - startTime;
        Scheduler::addProcessIntoQueue(queues, process, currentTime);
        if (usedTime < timeSlice && !queue.readyQueue.empty())
        {
            execute(queue, queues, process, currentTime, timeline, timeSlice - usedTime);
        }
    }
    else
    {
        currentTime += timeSlice;
        timeline.push_back({startTime, currentTime, queue.qid, p.pid});
        p.remainingTime -= timeSlice;
    }
}

void SRTNStrategy::execute(CPUQueue &queue, vector<CPUQueue> &queues, vector<Process> &processes, int &currentTime, vector<GanttEntry> &timeline, int timeSlice)
{
    if (queue.readyQueue.empty())
        return;
    int usedTime = 0;
    while (usedTime < timeSlice && !queue.readyQueue.empty())
    {
        int min = 0;
        for (int i = 1; i < queue.readyQueue.size(); i++)
        {
            int index1 = queue.readyQueue[i];
            int index2 = queue.readyQueue[min];
            if (processes[index1].remainingTime < processes[index2].remainingTime)
            {
                min = i;
            }
        }
        int indexProcess = queue.readyQueue[min];
        Process &p = processes[indexProcess];
        int startTime = currentTime;
        currentTime++;
        p.remainingTime--;
        usedTime++;
        timeline.push_back({startTime, currentTime, queue.qid, p.pid});
        if (p.remainingTime == 0)
        {
            p.completionTime = currentTime;
            p.isInQueue = false;
            queue.readyQueue.erase(queue.readyQueue.begin() + min);
        }

        Scheduler::addProcessIntoQueue(queues, processes, currentTime);
    }
}
void Scheduler::calculateMetrics()
{
    for (auto &p : processes)
    {
        p.turnaroundTime = p.completionTime - p.arrivalTime;
        p.waitingTime = p.completionTime - p.burstTime - p.arrivalTime;
    }
}
//KB
bool Scheduler::parseInputFromString(
    const std::string &content,
    std::vector<CPUQueue> &queues,
    std::vector<Process> &processes)
{
    return false;
}

SimulationResult Scheduler::run(
    std::vector<CPUQueue> &&queues,
    std::vector<Process> &&processes)
{
    return SimulationResult{};
}