#include <iostream>
#include "MDPModel.h"
#include "ModelConf.h"
#include <vector>
#include "Complex.h"
#include <chrono>

#include "stdlib.h"
#include "stdio.h"
#include "string.h"

int parseLine(char* line){
    // This assumes that a digit will be found and the line ends in " Kb".
    int i = strlen(line);
    const char* p = line;
    while (*p <'0' || *p > '9') p++;
    line[i-3] = '\0';
    i = atoi(p);
    return i;
}

int getValue(){ //Note: this value is in KB!
    FILE* file = fopen("/proc/self/status", "r");
    int result = -1;
    char line[128];

    while (fgets(line, 128, file) != NULL){
        if (strncmp(line, "VmRSS:", 6) == 0){
            result = parseLine(line);
            break;
        }
    }
    fclose(file);
    return result;
}
using namespace std::chrono;

using namespace std;

float avg(vector<float> v)
{
    float return_value = 0.0;
    int n = v.size();

    for (int i = 0; i < n; i++)
    {
        return_value += v[i];
    }
    return (return_value / (float)n);
}

pair<string, int> randomchoice(vector<pair<string, int>> v)
{
    float n = (float)v.size();
    float x = 1.0 / n;
    float r = myRand(0, 1);
    for (int i = 1; i < n + 1; i++)
    {
        if (r < x * i)
            return v[i - 1];
    }
}

int main()
{
    int num_tests = 5;
    int training_steps = 500;
    int eval_steps = 200;
    //vector<int> horizon{1, 5, 10, 25, 50, 75, 100, 250, 500, 750, 1000, 2500, 5000, 7500 ,10000, 25000, 50000, 100000 };
    int max_memory_used = 0;
    int load_period = 250;
    int MIN_VMS = 1;
    int MAX_VMS = 20;
    float epsilon = 0.7;
    string CONF_FILE = "/home/giannispapag/Thesis_Code/EffectiveMDP/Sim_Data/mdp_small.json";
    ModelConf conf(CONF_FILE);
    vector<float> total_rewards_results;

    pair<string, int> action;

    for (int i = 0; i < num_tests; i++)
    {
        ComplexScenario scenario(training_steps, load_period, 10, MIN_VMS, MAX_VMS);
        MDPModel model(conf.get_model_conf());
        model.set_state(scenario.get_current_measurements());
        float total_reward = 0.0;
        //cout << "TEST " << i+1 << endl;
        //cout << "----------------\n" ;
        auto start = high_resolution_clock::now();
        for (int time = 0; time < training_steps + eval_steps; time++)
        {
            float x = myRand(0, 1);
            if (x < epsilon && time < training_steps)
            {
                action = randomchoice(model.get_legal_actions());
            }
            else
            {
                action = model.suggest_action();
                //if (time >training_steps) cout << "Number of VMs: " << scenario.measurements["number_of_VMs"] << " Suggested Action: " << action.first << ", " << action.second << endl;
            }

            cout << "ITERATION " << time << endl;
            for (auto& s:model.states){
                cout << s.get_value() << endl;
            }
            cout << endl;

            float reward = scenario.execute_action(action);
            json meas = scenario.get_current_measurements();
            model.update(action, meas, reward);
            if (time % 50 == 1){
                model.value_iteration(0.1);
            }

            if (time > training_steps)
            {
                total_reward += reward;
                //cout << getValue() << "KB" << endl;
                if (getValue() > max_memory_used) max_memory_used = getValue();
                //cout << "Incoming Load: " <<  scenario.get_incoming_load() << " ,Current Capacity: " <<  scenario.get_current_capacity() << " ,Percentage of Read Load: " << meas["%_read_load"] <<  " ,Percentage of I/O per sec: " <<  meas["io_per_sec"] << endl;
            }
            //cout << "Reward: " << total_reward << endl;
        }
        auto stop = high_resolution_clock::now();
        auto duration = duration_cast<microseconds>(stop - start);
        //cout << "Total Reward after " << horizon[i] << " steps long horizon: " << total_reward << ". Total duration: "<< duration.count() * 0.000001<< " seconds. Maximum memory used: " << max_memory_used/1024.0 << " MB"<< endl;
        cout << eval_steps << "," << total_reward << "," << duration.count() * 0.000001 << "," << max_memory_used/1000.0 << endl;
        //cout << "Total Reward after evaluation: " << total_reward << endl;
        //cout << endl;
        total_rewards_results.push_back(total_reward);
    }
    cout << '\n';
    cout << "Total results after running " << num_tests << " tests" << endl;
    cout << "Average total rewards : " << avg(total_rewards_results) << endl;
    cout << '\n';
    //cout << duration.count() << " microseconds" << endl;
}