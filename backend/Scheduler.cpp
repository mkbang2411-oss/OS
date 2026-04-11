#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <memory>
#include "Scheduler.h"

using namespace std;

bool Scheduler::isAllDone() const
{
    for (auto &p : processes)
    {
        if (p.remainingTime > 0)
            return false;
    }
    return true;
}

void Scheduler::addProcessIntoQueue(vector<CPUQueue> &queues, vector<Process> &processes, int currentTime)
{
    for (int i = 0; i < (int)processes.size(); i++)
    {
        Process &p = processes[i];
        if (!p.isInQueue && p.arrivalTime <= currentTime && p.remainingTime > 0)
        {
            queues[p.queueIndex].readyQueue.push_back(i);
            p.isInQueue = true;
        }
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

SimulationResult Scheduler::run()
{
    int currentTime = 0;
    int currentQueue = 0;

    while (!isAllDone())
    {
        addProcessIntoQueue(queues, processes, currentTime);

        bool executed = false;

        for (int count = 0; count < (int)queues.size(); count++)
        {
            int i = (currentQueue + count) % (int)queues.size();

            if (!queues[i].readyQueue.empty())
            {
                queues[i].strategy->execute(queues[i], queues, processes, currentTime, timeline, queues[i].timeSlice);
                currentQueue = (i + 1) % (int)queues.size();
                executed = true;

                addProcessIntoQueue(queues, processes, currentTime);
                break;
            }
        }

        if (!executed)
        {
            currentTime++;
            addProcessIntoQueue(queues, processes, currentTime);
        }
    }

    calculateMetrics();

    SimulationResult result;
    double totalTurnaround = 0, totalWaiting = 0;
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

SimulationResult Scheduler::run(vector<CPUQueue> &&queues, vector<Process> &&processes)
{
    Scheduler s(move(queues), move(processes));
    return s.run();
}

void SJFStrategy::execute(CPUQueue &queue, vector<CPUQueue> &queues, vector<Process> &process,
                          int &currentTime, vector<GanttEntry> &timeline, int timeSlice)
{
    if (queue.readyQueue.empty())
        return;

    // Tìm process ngắn nhất
    int minIdx = 0;
    for (int i = 1; i < (int)queue.readyQueue.size(); i++)
    {
        if (process[queue.readyQueue[i]].remainingTime < process[queue.readyQueue[minIdx]].remainingTime)
            minIdx = i;
    }

    int indexProcess = queue.readyQueue[minIdx];
    Process &p = process[indexProcess];
    int startTime = currentTime;

    if (p.remainingTime <= timeSlice)
    {
        // Chạy hết process
        currentTime += p.remainingTime;
        int usedTime = currentTime - startTime;
        timeline.push_back({startTime, currentTime, queue.qid, p.pid});

        p.remainingTime = 0;
        p.completionTime = currentTime;
        p.isInQueue = false;
        queue.readyQueue.erase(queue.readyQueue.begin() + minIdx);

        // Còn dư thời gian → nạp thêm process mới rồi chạy tiếp
        Scheduler::addProcessIntoQueue(queues, process, currentTime);
        if (usedTime < timeSlice && !queue.readyQueue.empty())
        {
            execute(queue, queues, process, currentTime, timeline, timeSlice - usedTime);
        }
    }
    else
    {
        // Chạy đủ timeSlice rồi dừng (chưa xong)
        currentTime += timeSlice;
        timeline.push_back({startTime, currentTime, queue.qid, p.pid});
        p.remainingTime -= timeSlice;
    }
}

void SRTNStrategy::execute(CPUQueue &queue, vector<CPUQueue> &queues, vector<Process> &processes,
                           int &currentTime, vector<GanttEntry> &timeline, int timeSlice)
{
    if (queue.readyQueue.empty())
        return;

    int usedTime = 0;
    while (usedTime < timeSlice && !queue.readyQueue.empty())
    {
        int minIdx = 0;
        for (int i = 1; i < (int)queue.readyQueue.size(); i++)
        {
            if (processes[queue.readyQueue[i]].remainingTime < processes[queue.readyQueue[minIdx]].remainingTime)
                minIdx = i;
        }

        int indexProcess = queue.readyQueue[minIdx];
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
            queue.readyQueue.erase(queue.readyQueue.begin() + minIdx);
        }

        Scheduler::addProcessIntoQueue(queues, processes, currentTime);
    }
}

bool Scheduler::parseInputFromString(const string &content,
                                     vector<CPUQueue> &queues,
                                     vector<Process> &processes)
{
    queues.clear();
    processes.clear();

    istringstream ss(content);
    string line;

    // Helper: bỏ \r (Windows line ending) và skip dòng trống
    auto nextLine = [&](string &l) -> bool
    {
        while (getline(ss, l))
        {
            if (!l.empty() && l.back() == '\r')
                l.pop_back();
            if (!l.empty())
                return true;
        }
        return false;
    };

    // Helper: uppercase string
    auto toUpper = [](string s)
    {
        for (char &c : s)
            c = (char)toupper((unsigned char)c);
        return s;
    };

    // ── Bước 1: dòng đầu tiên = số queue N ──
    if (!nextLine(line))
        return false;
    int numQueues = 0;
    try
    {
        numQueues = stoi(line);
    }
    catch (...)
    {
        return false;
    }
    if (numQueues <= 0)
        return false;

    // ── Bước 2: đọc N dòng queue ──
    //  Format: Q1 8 SRTN  →  QID  TimeSlice  Algorithm
    for (int i = 0; i < numQueues; i++)
    {
        if (!nextLine(line))
            return false;

        istringstream ls(line);
        string qid, algo;
        int timeSlice;
        if (!(ls >> qid >> timeSlice >> algo))
            return false;

        algo = toUpper(algo);

        unique_ptr<SchedulingStrategy> strat;
        if (algo == "SJF")
            strat = make_unique<SJFStrategy>();
        else if (algo == "SRTN")
            strat = make_unique<SRTNStrategy>();
        else
            return false;

        CPUQueue q(qid, timeSlice, move(strat));
        q.algorithmName = algo;
        queues.push_back(move(q));
    }

    // ── Bước 3: đọc các dòng process đến hết file ──
    //  Format: P1 0 12 Q1  →  PID  ArrivalTime  BurstTime  QueueID
    while (nextLine(line))
    {
        istringstream ls(line);
        string pid, queueID;
        int arrival, burst;
        if (!(ls >> pid >> arrival >> burst >> queueID))
            continue; // bỏ qua dòng lỗi

        // Tìm index của queue theo tên (Q1 → 0, Q2 → 1...)
        int qIndex = -1;
        for (int i = 0; i < (int)queues.size(); i++)
        {
            if (queues[i].qid == queueID)
            {
                qIndex = i;
                break;
            }
        }
        if (qIndex == -1)
            return false; // tên queue không tồn tại

        processes.emplace_back(pid, arrival, burst, qIndex);
    }

    if (processes.empty())
        return false;
    return true;
}