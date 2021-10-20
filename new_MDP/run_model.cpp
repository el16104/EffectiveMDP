#include <iostream>
#include "FiniteMDPModel.h"
#include "ModelConf.h"
#include <vector>
#include "Complex.h"
#include <chrono>
#include <sstream>

#include "stdlib.h"
#include "stdio.h"
#include "string.h"

int parseLine2(char* line){
    // This assumes that a digit will be found and the line ends in " Kb".
    int i = strlen(line);
    const char* p = line;
    while (*p <'0' || *p > '9') p++;
    line[i-3] = '\0';
    i = atoi(p);
    return i;
}

int getValue2(){ //Note: this value is in KB!
    FILE* file = fopen("/proc/self/status", "r");
    int result = -1;
    char line[128];

    while (fgets(line, 128, file) != NULL){
        if (strncmp(line, "VmRSS:", 6) == 0){
            result = parseLine2(line);
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


pair<string, int> randomchoice(vector<pair<string, int>> v, FiniteMDPModel &model)
{
    float n = (float)v.size();
    float x = 1.0 / n;
    //float r = myRand(0, 1);
    float r = model.unif(model.eng);
    for (int i = 1; i < n + 1; i++)
    {
        if (r < x * i)
            return v[i - 1];
    }
    return v[0];
}

int main(int argc, char *argv[])
{
    int horizon = 100;
    string algorithm_type = "none";
    int seed = 21;

    if (argc >1) {
        algorithm_type = argv[1];
    }

    if (argc == 4){
        std::size_t pos;
        horizon = std::stoi(argv[2], &pos);
        seed = std::stoi(argv[3], &pos);
    }


    int training_steps = 10000;
    int max_memory_used = 0;
    int load_period = 250;
    int MIN_VMS = 1;
    int MAX_VMS = 20;
    float epsilon = 0.7;
    string CONF_FILE = "/home/giannispapag/Thesis_Code/EffectiveMDP/Sim_Data/mdp_small.json";
    ModelConf conf(CONF_FILE);
    float total_rewards_results[5][11];
    for (int i=0; i<5; i++){
        for (int j=0; j<11; j++){
            total_rewards_results[i][j] = 0.0;
        }
    }

    ComplexScenario scenario(5000, load_period, 10, MIN_VMS, MAX_VMS);
    FiniteMDPModel model(conf.get_model_conf(), seed);
    model.set_state(scenario.get_current_measurements());
    float total_reward = 0.0;
    pair<string, int> action;
    //TRAIN THE MODEL

    for (int time = 0; time < training_steps; time++){

        float x = model.unif(model.eng);
        if (x < epsilon)
        {
            action = randomchoice(model.get_legal_actions(), model);
        }
        else
        {
            action = model.suggest_action();
        }
        float reward = scenario.execute_action(action);
        json meas = scenario.get_current_measurements();
        model.update(action, meas, reward);
        if (time % 500 == 1){
            model.value_iteration(0.1);
        }
    }
    model.initial_state_num = model.current_state_num;

    if (algorithm_type == "infinite"){
    cout << "INFINITE MDP: " << endl;
    auto start = high_resolution_clock::now();
    model.resetValueFunction();
    model.value_iteration(0.1, false);
    for (int time = training_steps; time < training_steps + horizon; time++)
    {
        if (time == training_steps) model.expected_reward = model.states[model.initial_state_num].value;
        model.takeAction(true);
        if (getValue2() > max_memory_used) max_memory_used = getValue2();
    }
    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(stop - start);
    cout << horizon << "," << model.total_reward << "," << duration.count() * 0.000001 << "," << max_memory_used/1000.0 << endl; 
    cout << "Expected reward: " << model.expected_reward << " vs. Actual reward: " << model.total_reward << endl;  
    cout << endl;
    max_memory_used = 0.0;
    }
    else if (algorithm_type == "tree"){
    cout << "FINITE MDP MODEL (TREE): " << endl;
    auto start = high_resolution_clock::now();
    model.traverseTree(1, horizon);
    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(stop - start);
    cout << horizon << "," << model.total_reward << "," << duration.count() * 0.000001 << "," << model.max_memory_used/1000.0 << endl;
    cout << "Expected reward: " << model.expected_reward << " vs. Actual reward: " << model.total_reward << endl;  
    cout << endl;
    }
    else if (algorithm_type == "naive"){
    cout << "FINITE MDP MODEL (NAIVE): " << endl;
    auto start = high_resolution_clock::now();
    model.simpleEvaluation(horizon);
    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(stop - start);
    cout << horizon << "," << model.total_reward << "," << duration.count() * 0.000001 << "," << model.max_memory_used/1000.0 << endl;
    cout << "Expected reward: " << model.expected_reward << " vs. Actual reward: " << model.total_reward << endl;  
    cout << endl;
    }
    else if (algorithm_type == "inplace"){
    cout << "FINITE MDP MODEL (IN-PLACE): " << endl;
    auto start = high_resolution_clock::now();
    model.naiveEvaluation(horizon);
    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(stop - start);
    cout << horizon << "," << model.total_reward << "," << duration.count() * 0.000001 << "," << model.max_memory_used/1000.0 << endl;
    cout << "Expected reward: " << model.expected_reward << " vs. Actual reward: " << model.total_reward << endl;  
    cout << endl;
    }
    else if (algorithm_type == "root"){
    cout << "FINITE MDP MODEL (SQUARE ROOT): " << endl;
    auto start = high_resolution_clock::now();
    model.rootEvaluation(horizon);
    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(stop - start);
    cout << horizon << "," << model.total_reward << "," << duration.count() * 0.000001 << "," << model.max_memory_used/1000.0 << endl;
    cout << "Expected reward: " << model.expected_reward << " vs. Actual reward: " << model.total_reward << endl;  
    cout << endl;
    }
    else return 0;
    
}