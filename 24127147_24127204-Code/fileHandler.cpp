#include <iostream>
#include <sstream>
#include <fstream>
#include <memory>
#include <iomanip>
#include <algorithm>

#include "Scheduler.h"
#include "fileHandler.h"
using namespace std;

void FileHandler::readInput(
    const string &filename,
    vector<CPUQueue> &queues,
    vector<Process> &processes)
{
    ifstream fin(filename);

    if (!fin.is_open())
    {
        cerr << "Cannot open file";
        return;
    }

    int n;
    fin >> n;

    // Doc thong tin queue
    for (int i = 0; i < n; i++)
    {
        string qid, policy;
        int timeSlice;

        fin >> qid >> timeSlice >> policy;

        unique_ptr<SchedulingStrategy> Policy;

        if (policy == "SRTN")
        {
            Policy = make_unique<SRTNStrategy>();
        }
        else if (policy == "SJF")
        {
            Policy = make_unique<SJFStrategy>();
        }
        else
        {
            cerr << "The policy is invalid";
            return;
        }

        queues.emplace_back(qid, timeSlice, move(Policy));
    }

    // Doc thong tin Process
    string pid;
    while (fin >> pid)
    {
        int arrivalTime, burstTime;
        string qID;
        fin >> arrivalTime >> burstTime >> qID;

        int qIndex = -1;
        for (int j = 0; j < queues.size(); j++)
        {
            if (queues[j].qid == qID)
            {
                qIndex = j;
                break;
            }
        }

        if (qIndex == -1)
        {
            cerr << "Process has invalid queue";
            continue;
        }

        processes.emplace_back(pid, arrivalTime, burstTime, qIndex);
    }

    fin.close();
}

// canh giua chuo^~i trong 1 o
static string centerStr(string s, int w)
{
    int len = s.size();
    if (len >= w)
        return s.substr(0, w);

    int left = (w - len) / 2;
    int right = w - len - left;

    string res = "";
    res += string(left, ' ');
    res += s;
    res += string(right, ' ');

    return res;
}

static vector<GanttEntry> mergeTimeline(vector<GanttEntry> raw)
{
    vector<GanttEntry> result;

    for (int i = 0; i < raw.size(); i++)
    {
        if (result.size() > 0)
        {
            GanttEntry &last = result.back();

            // neu pid + queue giong nhau va thoi gian lien tiep
            if (last.pid == raw[i].pid && last.queueID == raw[i].queueID && last.end == raw[i].start)
            {
                last.end = raw[i].end;
                continue;
            }
        }

        result.push_back(raw[i]);
    }

    return result;
}

static vector<Block> makeBlocks(vector<string> occupy, vector<int> ticks)
{
    vector<Block> blocks;

    int seg = ticks.size() - 1;

    for (int i = 0; i < seg; i++)
    {
        string label = occupy[ticks[i]];

        if (blocks.size() > 0 && blocks.back().label == label)
        {
            blocks.back().span++;
        }
        else
        {
            Block b;
            b.label = label;
            b.span = 1;
            blocks.push_back(b);
        }
    }

    return blocks;
}

// in 1 hangg
void printRow(ofstream &fout, string label, bool mainRow, vector<Block> blocks, int labelW, int cellW)
{
    char h = mainRow ? '=' : '-';
    char v = mainRow ? '#' : '|';
    char c = mainRow ? '#' : '+';

    // dong tren
    fout << string(labelW, ' ');
    for (int i = 0; i < blocks.size(); i++)
    {
        int width = blocks[i].span * cellW + (blocks[i].span - 1);
        fout << c << string(width, h);
    }
    fout << c << "\n";

    // dong label
    fout << left << setw(labelW) << label;

    for (int i = 0; i < blocks.size(); i++)
    {
        int width = blocks[i].span * cellW + (blocks[i].span - 1);
        fout << v << centerStr(blocks[i].label, width);
    }
    fout << v << "\n";

    // dong duoi
    fout << string(labelW, ' ');
    for (int i = 0; i < blocks.size(); i++)
    {
        int width = blocks[i].span * cellW + (blocks[i].span - 1);
        fout << c << string(width, h);
    }
    fout << c << "\n\n";
}

void FileHandler::writeOutput(const string &filename, const SimulationResult &result)
{
    ofstream fout(filename);

    if (!fout.is_open())
    {
        cout << "Cannot open file " << filename << endl;
        return;
    }

    // lay danh sach cac sub queue
    vector<string> subQueues;

    for (int i = 0; i < result.timeline.size(); i++)
    {
        string qid = result.timeline[i].queueID;
        bool found = false;

        for (int j = 0; j < subQueues.size(); j++)
        {
            if (subQueues[j] == qid)
            {
                found = true;
                break;
            }
        }

        if (!found)
            subQueues.push_back(qid);
    }

    int numSub = subQueues.size();

    // tim THOI GIAN ket thuc cuoi
    int totalEnd = 0;

    for (int i = 0; i < result.timeline.size(); i++)
    {
        if (result.timeline[i].end > totalEnd)
            totalEnd = result.timeline[i].end;
    }

    // ma?ng luu process dang chay tren tung queue
    vector<vector<string>> qOccupy;

    for (int i = 0; i < numSub; i++)
    {
        vector<string> temp(totalEnd, "");
        qOccupy.push_back(temp);
    }

    for (int i = 0; i < result.timeline.size(); i++)
    {
        string qid = result.timeline[i].queueID;

        int qi = -1;
        for (int j = 0; j < subQueues.size(); j++)
        {
            if (subQueues[j] == qid)
            {
                qi = j;
                break;
            }
        }

        for (int t = result.timeline[i].start; t < result.timeline[i].end; t++)
        {
            qOccupy[qi][t] = result.timeline[i].pid;
        }
    }

    // QUEUE dang su dung CPU
    vector<string> cpuOccupy(totalEnd, "");

    for (int i = 0; i < result.timeline.size(); i++)
    {
        for (int t = result.timeline[i].start; t < result.timeline[i].end; t++)
        {
            cpuOccupy[t] = result.timeline[i].queueID;
        }
    }

    // tim cac moc thoi gian
    vector<int> ticks;
    ticks.push_back(0);

    for (int t = 1; t < totalEnd; t++)
    {
        if (cpuOccupy[t] != cpuOccupy[t - 1])
            ticks.push_back(t);
    }

    for (int qi = 0; qi < numSub; qi++)
    {
        for (int t = 1; t < totalEnd; t++)
        {
            if (qOccupy[qi][t] != qOccupy[qi][t - 1])
                ticks.push_back(t);
        }
    }

    ticks.push_back(totalEnd);

    sort(ticks.begin(), ticks.end());
    ticks.erase(unique(ticks.begin(), ticks.end()), ticks.end());

    int numSeg = ticks.size() - 1;

    // tinh do rong o
    int pidW = 2;

    for (int i = 0; i < result.processes.size(); i++)
    {
        if (result.processes[i].pid.size() > pidW)
            pidW = result.processes[i].pid.size();
    }

    for (int i = 0; i < subQueues.size(); i++)
    {
        if (subQueues[i].size() > pidW)
            pidW = subQueues[i].size();
    }

    int cellW = pidW + 2;
    if (cellW < 4)
        cellW = 4;

    int labelW = 5;

    for (int i = 0; i < subQueues.size(); i++)
    {
        if (subQueues[i].size() > labelW)
            labelW = subQueues[i].size();
    }

    labelW += 2;

    fout << "================== CPU SCHEDULING DIAGRAM ==================\n\n";

    fout << string(labelW, ' ');

    for (int s = 0; s < numSeg; s++)
    {
        string ts = to_string(ticks[s]);
        fout << ts;

        int gap = cellW + 1 - ts.size();

        if (gap > 0)
            fout << string(gap, ' ');
    }

    fout << ticks.back() << endl;

    // in dong CPU
    printRow(fout, "CPU", true, makeBlocks(cpuOccupy, ticks), labelW, cellW);

    // in cac queue
    for (int i = 0; i < numSub; i++)
    {
        vector<Block> blocks = makeBlocks(qOccupy[i], ticks);
        printRow(fout, subQueues[i], false, blocks, labelW, cellW);
    }

    fout << "\n================ PROCESS STATISTICS ================\n";
    fout << endl;

    const int cw[] = {10, 9, 7, 12, 12, 9};

    fout << left
         << setw(cw[0]) << "Process"
         << setw(cw[1]) << "Arrival"
         << setw(cw[2]) << "Burst"
         << setw(cw[3]) << "Completion"
         << setw(cw[4]) << "Turnaround"
         << setw(cw[5]) << "Waiting" << endl;

    int totalW = cw[0] + cw[1] + cw[2] + cw[3] + cw[4] + cw[5];

    fout << string(totalW, '-') << endl;

    for (int i = 0; i < result.processes.size(); i++)
    {
        const Process &p = result.processes[i];

        fout << left
             << setw(cw[0]) << p.pid
             << setw(cw[1]) << p.arrivalTime
             << setw(cw[2]) << p.burstTime
             << setw(cw[3]) << p.completionTime
             << setw(cw[4]) << p.turnaroundTime
             << setw(cw[5]) << p.waitingTime
             << endl;
    }

    fout << string(totalW, '-') << endl;
    fout << endl;

    fout << fixed << setprecision(1);
    fout << "Average Turnaround Time : " << result.avgTurnaround << endl;
    fout << "Average Waiting Time    : " << result.avgWaiting << endl;

    fout << string(totalW, '=') << endl;

    fout.close();
}