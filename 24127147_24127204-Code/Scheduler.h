#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <memory>

using namespace std;

class CPUQueue;

struct Process
{
    string pid;
    int arrivalTime;   // Thoi diem bat dau vao
    int burstTime;     // Thoi gian su dung
    int remainingTime; // Thoi gian chua su dung xong
    int queueIndex;

    int completionTime = 0; // Thoi diem ket thuc
    int turnaroundTime = 0;
    int waitingTime = 0;

    bool isInQueue = false; // Flag biet da ton tai trong queue

    Process(string id, int arrival, int burst, int qIndex)
    : pid(id), arrivalTime(arrival), burstTime(burst),
      remainingTime(burst), queueIndex(qIndex) {}
};

struct GanttEntry // Quan li các P trong CPU
{
    int start;
    int end;
    string queueID;
    string pid;
};

struct SimulationResult // Bieu dien ket qua
{
    vector<Process> processes;
    vector<GanttEntry> timeline;
    double avgTurnaround = 0;
    double avgWaiting = 0;
};

class SchedulingStrategy // Cac thuat toan chia CPU
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
        int timeSlice) override; // Ngoc Minh -> Nho reset isInQueue
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
        int timeSlice) override; // Ngoc Minh -> Nho reset isInQueue
};

class CPUQueue
{
public:
    string qid;
    int timeSlice; // Thoi gian su dung CPU trong RoundRobin
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
    vector<GanttEntry> timeline; // Danh dau cac thoi diem

    void calculateMetrics(); // Ngoc Minh -> Tinh turnaround time va waiting time cua tung Process

public:
    Scheduler(vector<CPUQueue> &&q, vector<Process> &&p)
    : queues(move(q)), processes(move(p)) {}

    SimulationResult run(); // Khanh Bang

    bool isAllDone() const;

    static void addProcessIntoQueue(vector<CPUQueue> &queues, vector<Process> &processes, int currentTime);
};