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

    Process(string id, int arrival, int burst, int qIndex);
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

class CPUQueue
{
public:
    string qid;
    int timeSlice;
    unique_ptr<SchedulingStrategy> strategy;

    vector<int> readyQueue;

    CPUQueue(string id, int slice, unique_ptr<SchedulingStrategy> strat)
        : qid(id), timeSlice(slice), strategy(move(strat)) {}
};

class SchedulingStrategy // Cac thuat toan chia CPU
{
public:
    virtual void execute(
        CPUQueue &queue,
        vector<Process> &processes,
        int &currentTime,
        vector<GanttEntry> &timeline) = 0;

    virtual ~SchedulingStrategy() {}
};

class SJFStrategy : public SchedulingStrategy
{
public:
    void execute(
        CPUQueue &queue,
        vector<Process> &processes,
        int &currentTime,
        vector<GanttEntry> &timeline) override; // Ngoc Minh
};

class SRTNStrategy : public SchedulingStrategy
{
public:
    void execute(
        CPUQueue &queue,
        vector<Process> &processes,
        int &currentTime,
        vector<GanttEntry> &timeline) override; // Ngoc Minh
};

class Scheduler // OS gia lap
{
private:
    vector<CPUQueue> queues;
    vector<Process> processes;
    vector<GanttEntry> timeline;

    void calculateMetrics(); // Ngoc Minh

public:
    Scheduler(
        const vector<CPUQueue> &q,
        const vector<Process> &p);

    SimulationResult run(); // Khanh Bang
};