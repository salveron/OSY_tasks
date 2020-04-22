#ifndef __PROGTEST__

#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <climits>
#include <cfloat>
#include <cassert>
#include <cmath>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <numeric>
#include <vector>
#include <set>
#include <list>
#include <map>
#include <unordered_set>
#include <unordered_map>
#include <queue>
#include <stack>
#include <deque>
#include <memory>
#include <functional>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include "progtest_solver.h"
#include "sample_tester.h"

using namespace std;
#endif /* __PROGTEST__ */

#include <string_view>

class CSentinelHacker{
public:
    CSentinelHacker()
      : all_fragments_sent (false),
        all_work_completed (false){}


    static bool SeqSolve(const vector<uint64_t> & fragments, CBigInt & res){
        vector<pair<const uint8_t *, size_t>> bitfields;
        uint32_t number_of_payloads = FindPermutations(
                fragments.data(),
                fragments.size(),
                [&bitfields](const uint8_t * payload, size_t num) {
                    int size_to_copy = (int)(num / 8) + 1 - 4;
                    auto * copied_payload = new uint8_t [size_to_copy];
                    for (int i = 0; i < size_to_copy; i++)
                        copied_payload[i] = payload[i + 4];
                    bitfields.emplace_back(copied_payload, num - 32);
                });

        if (number_of_payloads == 0)
            return false;

        CBigInt maxResult;
        for (auto & pair : bitfields) {
            CBigInt tmp = CountExpressions(pair.first, pair.second);
            if (tmp.CompareTo(maxResult) > 0)
                maxResult = tmp;
            delete [] pair.first;
        }

        res = maxResult;
        return true;
    }


    void AddReceiver(AReceiver x){ receivers.push_back(x); }
    void AddTransmitter(ATransmitter x){ transmitters.push_back(x); }


    void AddFragment(uint64_t x){
        uint32_t fragment_id = x >> SHIFT_MSG_ID;
        {
            unique_lock<mutex> lk (rw_mutex);
            fragments[fragment_id].push_back(x);
            to_process.emplace_back(fragments[fragment_id]);
        }
        rw_cv.notify_one();
    }


    void Start(unsigned thrCount){

        unsigned i = 0;
        for (auto it = receivers.begin(); it != receivers.end(); ++it, i++)
            receiver_threads.emplace_back([=]{ ReceiverThreadFunction(i, *it); });

        i = 0;
        for (; i < thrCount; i++)
            worker_threads.emplace_back([=]{ WorkerThreadFunction(i + 1); });

        i = 0;
        for (auto it = transmitters.begin(); it != transmitters.end(); ++it, i++)
            transmitter_threads.emplace_back([=]{ TransmitterThreadFunction(i, *it); });
    }


    void Stop(){
        for (auto & receiver_thread : receiver_threads)
            receiver_thread.join();

        all_fragments_sent = true;
        rw_cv.notify_all();

        for (auto & worker_thread : worker_threads)
            worker_thread.join();

        all_work_completed = true;
        ic_it = incomplete_ids.begin();
        wt_cv.notify_all();

        for (auto & transmitter_thread : transmitter_threads)
            transmitter_thread.join();
    }

private:
    map<uint32_t, vector<uint64_t>> fragments;                       /* fragments grouped by their ID */
    deque<vector<uint64_t>> to_process;                              /* queue of vectors to be given to workers */
    deque<pair<uint32_t, string>> to_send;                           /* queue of pairs to be sent by transmitters */

    set<uint32_t> incomplete_ids;                                    /* set of incomplete ids */
    bool all_fragments_sent;
    bool all_work_completed;
    set<uint32_t>::iterator ic_it;

    vector<AReceiver> receivers;
    vector<ATransmitter> transmitters;

    vector<thread> receiver_threads;
    vector<thread> worker_threads;
    vector<thread> transmitter_threads;

    mutex rw_mutex;                                                  /* receiver-worker mutex */
    condition_variable rw_cv;                                        /* receiver-worker condition variable */
    mutex wt_mutex;                                                  /* worker-transmitter mutex */
    condition_variable wt_cv;                                        /* worker-transmitter condition variable */
    mutex ic_mutex;                                                  /* mutex for the set of incomplete ids */

    void ReceiverThreadFunction(unsigned thID, const AReceiver & receiver){
        uint64_t fragment;
        while(receiver->Recv(fragment)){
            uint32_t fragment_id = fragment >> SHIFT_MSG_ID;
            {
                unique_lock<mutex> lk (rw_mutex);
                fragments[fragment_id].push_back(fragment);
                to_process.emplace_back(fragments[fragment_id]);
            }
            rw_cv.notify_one();
        }
    }


    void WorkerThreadFunction(unsigned thID){
        while(!to_process.empty() || !all_fragments_sent){
            vector<uint64_t> fragments_to_process;
            {
                unique_lock<mutex> lk(rw_mutex);
                while (to_process.empty() && !all_fragments_sent){
                    rw_cv.wait(lk);
                }
                if (to_process.empty() && all_fragments_sent){
                    lk.unlock();
                    break;
                }
                fragments_to_process = to_process.front();
                to_process.pop_front();
            }
            uint32_t group_id = fragments_to_process.front() >> SHIFT_MSG_ID;

            vector<pair<const uint8_t *, size_t>> bitfields;
            uint32_t number_of_payloads = FindPermutations(
                    fragments_to_process.data(),
                    fragments_to_process.size(),
                    [&bitfields](const uint8_t *payload, size_t num) {
                        int size_to_copy = (int)(num / 8) + 1 - 4;
                        auto * copied_payload = new uint8_t [size_to_copy];
                        for (int i = 0; i < size_to_copy; i++)
                            copied_payload[i] = payload[i + 4];
                        bitfields.emplace_back(copied_payload, num - 32);
                    });

            if (number_of_payloads == 0){
                {
                    unique_lock<mutex> lk(ic_mutex);
                    incomplete_ids.insert(group_id);
                }
                continue;
            }

            {
                unique_lock<mutex> lk (ic_mutex);
                incomplete_ids.erase(group_id);
            }

            string max_result = "0";
            for (auto &pair : bitfields) {
                string tmp = CountExpressions(pair.first, pair.second).ToString();
                printf("W%d: group %d ,candidate %s\n", thID, group_id, tmp.c_str());
                if (CBigInt(tmp).CompareTo(CBigInt(max_result)) > 0)
                    max_result = tmp;
                delete [] pair.first;
            }

            {
                unique_lock<mutex> lk (wt_mutex);
                to_send.emplace_back(group_id, max_result);
            }
            wt_cv.notify_one();
        }
    }


    void TransmitterThreadFunction(unsigned thID, const ATransmitter & transmitter){
        while(!to_send.empty() || !all_work_completed){
            uint32_t id_to_send;
            string count_to_send;
            {
                unique_lock<mutex> lk(wt_mutex);
                while (to_send.empty() && !all_work_completed){
                    wt_cv.wait(lk);
                }
                if (to_send.empty() && all_work_completed){
                    lk.unlock();
                    break;
                }
                id_to_send = to_send.front().first;
                count_to_send = to_send.front().second;
                to_send.pop_front();
            }
            transmitter->Send(id_to_send, CBigInt(count_to_send));
        }

        while(ic_it != incomplete_ids.cend()){
            uint32_t incomplete_id;
            {
                unique_lock<mutex> lk (ic_mutex);
                incomplete_id = *ic_it;
                ic_it++;
            }
            transmitter->Incomplete(incomplete_id);
        }
    }
};


//-------------------------------------------------------------------------------------------------
#ifndef __PROGTEST__

int main(void){
    using namespace std::placeholders;
    for (const auto & x : g_TestSets){
        CBigInt res;
        assert(CSentinelHacker::SeqSolve(x.m_Fragments, res));
        assert(CBigInt(x.m_Result).CompareTo(res) == 0);
    }

    CSentinelHacker test;
    auto trans = make_shared<CExampleTransmitter>();
    AReceiver recv = make_shared<CExampleReceiver>(
            initializer_list<uint64_t> {0x02230000000c, 0x071e124dabef, 0x02360037680e, 0x071d2f8fe0a1,
                                        0x055500150755});

    test.AddTransmitter(trans);
    test.AddReceiver(recv);
    test.Start(3);

    static initializer_list<uint64_t> t1Data = {0x071f6b8342ab, 0x0738011f538d, 0x0732000129c3, 0x055e6ecfa0f9,
                                                0x02ffaa027451, 0x02280000010b, 0x02fb0b88bc3e};
    thread t1(FragmentSender, bind(&CSentinelHacker::AddFragment, &test, _1), t1Data);

    static initializer_list<uint64_t> t2Data = {0x073700609bbd, 0x055901d61e7b, 0x022a0000032b, 0x016f0000edfb};
    thread t2(FragmentSender, bind(&CSentinelHacker::AddFragment, &test, _1), t2Data);
    FragmentSender(bind(&CSentinelHacker::AddFragment, &test, _1),
                   initializer_list<uint64_t> {0x017f4cb42a68, 0x02260000000d, 0x072500000025});
    t1.join();
    t2.join();
    test.Stop();
    assert(trans->TotalSent() == 4);
    assert(trans->TotalIncomplete() == 2);
    return 0;
}

#endif /* __PROGTEST__ */
