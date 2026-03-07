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

    if(!fin.is_open())
    {
        cerr<<"Cannot open file";
        return;
    }

    int n;
    fin>>n;

    //Doc thong tin queue
    for (int i=0;i<n;i++)
    {
        string qid, policy;
        int timeSlice;

        fin>>qid>>timeSlice>>policy;

        unique_ptr <SchedulingStrategy> Policy;

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
            cerr<<"The policy is invalid";
            return;
        }

        queues.emplace_back(qid, timeSlice, move(Policy));
    }

    //Doc thong tin Process
    string pid;
    while(fin>>pid)
    {
        int arrivalTime, burstTime;
        string qID;
        fin>>arrivalTime>>burstTime>>qID;

        int qIndex = -1;
        for (int j =0;j<queues.size();j++)
        {
            if (queues[j].qid == qID)
            {
                qIndex = j;
                break;
            }
        }
        
        if (qIndex == -1)
        {
            cerr<<"Process has invalid queue";
            continue;
        }

        processes.emplace_back(pid,arrivalTime, burstTime, qIndex);
    }

    fin.close();
} 

//Canh giua stirng Px, Qx -> la s
//Trong khoang rong w
static string centerStr(const string &s, int w)
{
    int pad = w - (int)s.size(); //so khoang trang can them
    if (pad <= 0) return s.substr(0, w);
    int lp = pad / 2;
    return string(lp, ' ') + s + string(pad - lp, ' ');  //Tao / ghe'p chuoi cuoi cung sau tinh toan
}

//gop o^ cua cac doan lien tiep
static vector<GanttEntry> mergeTimeline(const vector<GanttEntry> &raw)
{
    vector<GanttEntry> merged;
    for (const auto &e : raw)
    {
        //Dieu kien gop
        if (!merged.empty() &&
            merged.back().pid     == e.pid &&  // Cung 1 Process
            merged.back().queueID == e.queueID &&  //Cung 1 Queue
            merged.back().end     == e.start) // Thoi gian lien ke nhau
            merged.back().end = e.end;
        else
            merged.push_back(e); //Khong thoa thi them e moi
    }
    return merged;
}

// Một "block" trong 1 row: nội dung + span bao nhiêu global-segment
struct Block { string label; int span; };

// Từ occupancy array + global ticks → tạo danh sách Block cho 1 row
// Các segment liên tiếp có cùng label được gộp thành 1 block
static vector<Block> makeBlocks(
    const vector<string> &occupy,   // occupy[t] = label tại thời điểm t
    const vector<int>    &ticks)    // global tick list
{
    int numSeg = (int)ticks.size() - 1;
    vector<Block> blocks;
    for (int s = 0; s < numSeg; s++)
    {
        string cur = occupy[ticks[s]];
        if (!blocks.empty() && blocks.back().label == cur)
            blocks.back().span++;
        else
            blocks.push_back({cur, 1});
    }
    return blocks;
}

// ═════════════════════════════════════════════════════════════════════
//  WRITE OUTPUT
// ═════════════════════════════════════════════════════════════════════
void FileHandler::writeOutput(
    const string          &filename,
    const SimulationResult &result)
{
    ofstream fout(filename);
    if (!fout.is_open()) { cerr << "Cannot open output file: " << filename << "\n"; return; }

    // ── Sub-queue IDs in first-appearance order ───────────────────────
    vector<string> subQueues;
    for (const auto &e : result.timeline)
    {
        bool found = false;
        for (auto &q : subQueues) if (q == e.queueID) { found = true; break; }
        if (!found) subQueues.push_back(e.queueID);
    }
    int numSub = (int)subQueues.size();

    // ── Time range ────────────────────────────────────────────────────
    int totalEnd = 0;
    for (const auto &e : result.timeline) totalEnd = max(totalEnd, e.end);

    // ── Per-sub-queue occupancy [qi][t] ───────────────────────────────
    vector<vector<string>> qOccupy(numSub, vector<string>(totalEnd, ""));
    for (const auto &e : result.timeline)
    {
        int qi = (int)(find(subQueues.begin(), subQueues.end(), e.queueID) - subQueues.begin());
        for (int t = e.start; t < e.end; t++) qOccupy[qi][t] = e.pid;
    }

    // ── CPU occupancy [t] = queueID running ──────────────────────────
    vector<string> cpuOccupy(totalEnd, "");
    for (const auto &e : result.timeline)
        for (int t = e.start; t < e.end; t++) cpuOccupy[t] = e.queueID;

    // ── Global ticks = union of ALL change boundaries ─────────────────
    // (needed so all rows share the same column grid for alignment)
    vector<int> ticks;
    ticks.push_back(0);
    for (int t = 1; t < totalEnd; t++)
        if (cpuOccupy[t] != cpuOccupy[t-1]) ticks.push_back(t);
    for (int qi = 0; qi < numSub; qi++)
        for (int t = 1; t < totalEnd; t++)
            if (qOccupy[qi][t] != qOccupy[qi][t-1]) ticks.push_back(t);
    ticks.push_back(totalEnd);
    sort(ticks.begin(), ticks.end());
    ticks.erase(unique(ticks.begin(), ticks.end()), ticks.end());
    int numSeg = (int)ticks.size() - 1;

    // ── Cell & label widths ───────────────────────────────────────────
    int pidW = 2;
    for (const auto &p : result.processes) pidW = max(pidW, (int)p.pid.size());
    for (auto &q : subQueues)              pidW = max(pidW, (int)q.size());
    int cellW = max(pidW + 2, 4); // inner width of 1 base segment

    int labelW = 5;
    for (auto &q : subQueues) labelW = max(labelW, (int)q.size());
    labelW += 2;

    fout << "================== CPU SCHEDULING DIAGRAM ==================\n\n";

    // ── Time axis ─────────────────────────────────────────────────────
    fout << string(labelW, ' ');
    for (int s = 0; s < numSeg; s++)
    {
        string ts = to_string(ticks[s]);
        fout << ts;
        int gap = (cellW + 1) - (int)ts.size();
        if (gap > 0) fout << string(gap, ' ');
    }
    fout << to_string(ticks.back()) << "\n";

    // ── Draw one pipe row, with block-spanning ────────────────────────
    // A block with span=k occupies k segments → inner width = k*cellW + (k-1)*1
    //   because shared borders between adjacent cells collapse into 1
    // Visual:  +----+----+  (2 separate cells, 3 borders)
    //      vs  +----------+  (1 merged cell, 2 borders)
    //   merged inner = 2*cellW + 1  (one extra char for the eaten border)

    auto printPipeRow = [&](const string &label, bool isMain,
                            const vector<Block> &blocks)
    {
        char hB = isMain ? '=' : '-';
        char vB = isMain ? '#' : '|';
        char co = isMain ? '#' : '+';

        // top border  – draw a divider at every block boundary
        fout << string(labelW, ' ');
        for (const auto &b : blocks)
        {
            int innerW = b.span * cellW + (b.span - 1); // merged inner width
            fout << co << string(innerW, hB);
        }
        fout << co << "\n";

        // content row
        {
            ostringstream oss; oss << left << setw(labelW) << label;
            fout << oss.str();
        }
        for (const auto &b : blocks)
        {
            int innerW = b.span * cellW + (b.span - 1);
            fout << vB << centerStr(b.label, innerW);
        }
        fout << vB << "\n";

        // bottom border
        fout << string(labelW, ' ');
        for (const auto &b : blocks)
        {
            int innerW = b.span * cellW + (b.span - 1);
            fout << co << string(innerW, hB);
        }
        fout << co << "\n\n";
    };

    // CPU row
    printPipeRow("CPU", true,  makeBlocks(cpuOccupy, ticks));

    // Sub-queue rows
    for (int qi = 0; qi < numSub; qi++)
        printPipeRow(subQueues[qi], false, makeBlocks(qOccupy[qi], ticks));


    fout << "\n================ PROCESS STATISTICS ================\n";

    const int cw[] = {10, 9, 7, 12, 12, 9};
    const int totalW = cw[0]+cw[1]+cw[2]+cw[3]+cw[4]+cw[5];

    fout << left
         << setw(cw[0]) << "Process"  << setw(cw[1]) << "Arrival"
         << setw(cw[2]) << "Burst"    << setw(cw[3]) << "Completion"
         << setw(cw[4]) << "Turnaround" << setw(cw[5]) << "Waiting" << "\n";
    fout << string(totalW, '-') << "\n";

    for (const auto &p : result.processes)
        fout << left
             << setw(cw[0]) << p.pid          << setw(cw[1]) << p.arrivalTime
             << setw(cw[2]) << p.burstTime     << setw(cw[3]) << p.completionTime
             << setw(cw[4]) << p.turnaroundTime << setw(cw[5]) << p.waitingTime << "\n";

    fout << string(totalW, '-') << "\n";
    fout << fixed << setprecision(1);
    fout << "Average Turnaround Time : " << result.avgTurnaround << "\n";
    fout << "Average Waiting Time    : " << result.avgWaiting    << "\n";
    fout << string(totalW, '=') << "\n";

    fout.close();
}