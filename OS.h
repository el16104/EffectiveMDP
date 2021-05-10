#include <iostream>
#include <map>
#include <ctime>
#include <cstdlib>
#include <math.h> 

#define PI 3.14159265

using namespace std;

extern bool flag = true;

double myRand(double LO, double HI){
    if (flag){
        srand(static_cast <unsigned> (time(0)));
        flag = false;
    }

    double r3 = LO + static_cast <double> (rand()) /( static_cast <double> (RAND_MAX/(HI-LO)));
    return r3;
}


class Triplet{
    public:
        string a;
        string b;
        string c;
        Triplet(string a, string b, string c){
            a = a;
            b = b;
            c = c;
        }
};


class OSScenario{
    public:
        double time;
        int load_period;
        int MIN_VMS;
        int MAX_VMS;
        double last_load;
        map<string, double> measurements;

        OSScenario(int load_period=60, int init_vms=15, int min_vms=4, int max_vms=15){
            time = 0.0;
            load_period = load_period;
            MIN_VMS = min_vms;
            MAX_VMS = max_vms;
            last_load = -1.0;
            measurements = _get_measurements(init_vms);
        }

        map<string, double> _get_measurements(int num_vms){
            map<string,double> m;
            m.insert({ "number_of_VMs" , (double)num_vms});
            m.insert({ "incoming_load" , _get_load()});
            m.insert({ "next_load" , this->_get_next_load()});
            m.insert({ "%_CPU_usage" , myRand(0.0, 100.0)});
            m.insert({ "%_cached_RAM" , myRand(0.0, 0.3)});
            m.insert({"%_free_RAM", myRand(0.1, 0.3)});
            m.insert({"%_read_load", 1.0});
            m.insert({"%_read_throughput" , myRand(1.0, 1.001)});
            m.insert({"RAM_size", 1024.0});
            m.insert({"bytes_in" , myRand(320000.0, 350000.0)});
            m.insert({"bytes_out", myRand(2500000.0 , 3500000.0)});
            m.insert({"cpu", m["%_CPU_usage"] * 0.03});
            m.insert({"cpu_idle", myRand(0.0, 100.0)});
            m.insert({"cpu_nice", 0.0});
            m.insert({"cpu_system", myRand(0.0, 50.0)});
            m.insert({"cpu_user", myRand(0.0, 50.0)});
            m.insert({"cpu_wio", myRand(0.0, 4.0)});
            m.insert({"disk_free", myRand(0.0, 10.0)});
            m.insert({"io_reqs", myRand(0.0, 10.0)});
            m.insert({"load_fifteen" , myRand(0.0, 1.0)});
            m.insert({"load_five" , myRand(0.0, 1.0)});
            m.insert({"load_one" , myRand(0.0, 1.0)});
            m.insert({"mem_buffers" , myRand(50000.0, 100000.0)});
            m.insert({"mem_cached" , myRand(200000.0, 300000.0)});
            m.insert({"mem_free" , myRand(100000.0, 200000.0)});
            m.insert({"mem_shared" , 0.0});
            m.insert({"mem_total", 1017876.0});
            m.insert({"number_of_CPUs", 1.0});
            m.insert({"number_of_threads", myRand(0,30.0)});
            m.insert({"part_max_used", myRand(40.0,70.0)});
            m.insert({"pkts_in", myRand(2000.0, 5000.0)});
            m.insert({"pkts_out", myRand(2000.0, 5000.0)});
            m.insert({"proc_run", myRand(1.0, 10.0)});
            m.insert({"proc_total", myRand(10.0,500.0)});
            m.insert({"read_io_reqs", 0.0});
            m.insert({"read_latency", myRand(5.0, 20.0)});
            m.insert({"update_throughput", myRand(1.0, 2.0)});
            m.insert({"total_throughput", min(get_current_capacity(num_vms), m["incoming_load"])});
            m.insert({"read_throughput", m["total_throughput"] + myRand(1.0, 10.0)});
            m.insert({"storage_capacity" , 10.0});
            m.insert({"total_latency", m["read_latency"] + myRand(0.01, 0.1)});
            m.insert({"update_latency", myRand(0.01, 0.1)});
            m.insert({"write_io_reqs", m["io_reqs"]});

            return m;

        }

        double get_current_capacity(int vms = -1){
            if (vms==-1)
                vms = measurements["number_of_VMs"];
            int capacity = 8689 + 2322 * vms + myRand(-500, 500);

            return (double)capacity;
        }

        double _get_reward(pair<string,int> action){
            int vms = measurements["number_of_VMs"];
            int load = measurements["incoming_load"];
            int capacity = (int)get_current_capacity();
            int served_load = min(capacity, load);
            double reward = served_load - 800 * vms;

            return reward;
        }

        map<string, double> get_current_measurements(){
            return measurements;
        }

        double execute_action(pair<string, int> action){
            time++;
            last_load = measurements["incoming_load"];
            int num_vms = measurements["number_of_VMs"];
            string action_type = action.first;
            int action_value = action.second;
            if (action_type == "add_VMs")
                num_vms += action_value;
            if (action_type == "remove_VMs")
                num_vms -= action_value;
            if (num_vms < MIN_VMS)
                num_vms = MIN_VMS;
            if (num_vms > MAX_VMS)
                num_vms = MAX_VMS;

            measurements = _get_measurements(num_vms);
            double reward = _get_reward(action);

            return reward;

        }

        Triplet get_relevant_params(){
            Triplet x("number_of_VMs", "incoming_load", "next_load");
            return x;
        }

        double _get_load(){
            double x = 2.0 * PI * time / load_period;
            return (30000.0 + 12000.0 * sin(x));
        }

        double _get_next_load(){
            if (last_load == -1.0)
                return _get_load();
            else
                return 2*_get_load() - last_load;
        }

};