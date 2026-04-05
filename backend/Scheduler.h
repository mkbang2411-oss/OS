#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <memory>

using namespace std;

class CPUQueue; //

struct Process
{
    string pid;
    int arrivalTime;
    int burstTime;
    int remainingTime;
    int queueIndex;

    int completionTime = 0;
    int turnaroundTime = 0;
    int waitingTime = 0;

    bool isInQueue = false;

    Process(string id, int arrival, int burst, int qIndex)
        : pid(id), arrivalTime(arrival), burstTime(burst),
          remainingTime(burst), queueIndex(qIndex) {}
};

struct GanttEntry
{
    int start;
    int end;
    string queueID;
    string pid;
};

struct SimulationResult
{
    vector<Process> processes;
    vector<GanttEntry> timeline;
    double avgTurnaround = 0;
    double avgWaiting = 0;
};

class SchedulingStrategy
{
public:
    virtual void execute(
        CPUQueue &queue,
        vector<CPUQueue> &queues,
        vector<Process> &processes,
        int &currentTime,
        vector<GanttEntry> &timeline,
        int timeSlice) = 0;

    virtual ~SchedulingStrategy() {}
};

class SJFStrategy : public SchedulingStrategy
{
public:
    void execute(
        CPUQueue &queue,
        vector<CPUQueue> &queues,
        vector<Process> &processes,
        int &currentTime,
        vector<GanttEntry> &timeline,
        int timeSlice) override;
};

class SRTNStrategy : public SchedulingStrategy
{
public:
    void execute(
        CPUQueue &queue,
        vector<CPUQueue> &queues,
        vector<Process> &processes,
        int &currentTime,
        vector<GanttEntry> &timeline,
        int timeSlice) override;
};

class CPUQueue
{
public:
    string qid;
    string algorithmName; // Thêm so với project 1
    int timeSlice;
    unique_ptr<SchedulingStrategy> strategy;

    vector<int> readyQueue;

    CPUQueue(string id, int slice, unique_ptr<SchedulingStrategy> strat)
        : qid(id), timeSlice(slice), strategy(move(strat)) {}
};

class Scheduler // OS gia lap
{
private:
    vector<CPUQueue> queues;
    vector<Process> processes;
    vector<GanttEntry> timeline;

    void calculateMetrics();

public:
    Scheduler(vector<CPUQueue> &&q, vector<Process> &&p)
        : queues(move(q)), processes(move(p)) {}

    bool isAllDone() const;

    static void addProcessIntoQueue(vector<CPUQueue> &queues, vector<Process> &processes, int currentTime);

    ///////

    SimulationResult run(); // Tinh chinh lai ham run-> thay bang ham run duoi -> sua lai cach input + output file

    static SimulationResult run(vector<CPUQueue> &&queues, vector<Process> &&processes);

    //////

    static bool parseInputFromString(
        const string &content,
        vector<CPUQueue> &queues,
        vector<Process> &processes); // Doc file bang phuong phap khac -> Doc truc tiep tu cau truc FAT32 -> Khong doc ifstream
};