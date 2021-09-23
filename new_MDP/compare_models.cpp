#include <iostream>
#include "FiniteMDPModel.h"
#include "ModelConf.h"
#include <vector>
#include "Complex.h"
#include <chrono>

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
    int num_tests = 20;
    int training_steps = 5000;
    //int eval_steps = 80;
    //vector<int> horizon{1, 5, 10, 25, 50, 75, 100, 250, 500, 750, 1000, 2500, 5000, 7500 ,10000, 25000, 50000, 75000, 100000 };
    vector<int> horizon{100, 250, 500, 750, 1000, 2500, 5000, 7500 ,10000};
    //vector<int> horizon{100};
    vector<FiniteMDPModel> models;
    int max_memory_used = 0;
    int load_period = 250;
    int MIN_VMS = 1;
    int MAX_VMS = 20;
    float epsilon = 0.7;
    string CONF_FILE = "/home/giannispapag/Thesis_Code/EffectiveMDP/Sim_Data/mdp_small.json";
    ModelConf conf(CONF_FILE);
    float total_rewards_results[3][11];
    for (int i=0; i<3; i++){
        for (int j=0; j<11; j++){
            total_rewards_results[i][j] = 0.0;
        }
    }

    for (int test_number = 0; test_number < num_tests; test_number++){

    pair<string, int> action;
    for (int i = 0; i < horizon.size(); i++)
    {
        ComplexScenario scenario(training_steps, load_period, 10, MIN_VMS, MAX_VMS);
        FiniteMDPModel model(scenario, conf.get_model_conf());
        model.set_state(scenario.get_current_measurements());
        float total_reward = 0.0;
        //TRAIN THE MODEL
        for (int time = 0; time < training_steps; time++){
            float x = myRand(0, 1);
            if (x < epsilon)
            {
                action = randomchoice(model.get_legal_actions());
            }
            else
            {
                action = model.suggest_action();
            }
            float reward = scenario.execute_action(action);
            json meas = scenario.get_current_measurements();
            model.update(action, meas, reward);
            if (time % 500 == 1){
                model.value_iteration(0.01);
            }
        }

        //COPY THE CREATED MODEL ONCE FOR EACH ALGORITHM
        //model.set_state(scenario.get_current_measurements());
        if ( i ==0 && test_number == 0){
            for (int k = 0; k < 3; k++){
                models.push_back(model);
        }
        }
        else{
            for (int k = 0; k < 3; k++){
            models[k] = model;
        }
        }
        ComplexScenario s1 = scenario;
        ComplexScenario s2 = scenario;
        models[1].scenario = &s1;
        models[2].scenario = &s2;
        models[1].scenario->time = training_steps;
        models[2].scenario->time = training_steps;
        scenario.time = training_steps;

        //INFINITE MDP MODEL
        cout << "INFINITE MDP: " << endl;
        auto start = high_resolution_clock::now();
        for (int time = training_steps; time < training_steps + horizon[i] + 1; time++)
        {
            models[0].value_iteration(0.0001);
            action = models[0].suggest_action();
            float reward = scenario.execute_action(action);
            json meas = scenario.get_current_measurements();
            models[0].update_finite(action,meas,reward);
            total_reward += reward;
            if (getValue2() > max_memory_used) max_memory_used = getValue2();
        }
        auto stop = high_resolution_clock::now();
        auto duration = duration_cast<microseconds>(stop - start);
        //cout << "Total Reward after " << horizon[i] << " steps long horizon: " << total_reward << ". Total duration: "<< duration.count() * 0.000001<< " seconds. Maximum memory used: " << max_memory_used/1024.0 << " MB"<< endl;
        cout << horizon[i] << "," << total_reward << "," << duration.count() * 0.000001 << "," << max_memory_used/1000.0 << endl;
        total_rewards_results[0][i] += total_reward;
        cout << endl;

        //cout << "NUMBER OF VMS: " << models[1].scenario->measurements["number_of_VMs"] << endl;
        cout << "FINITE MDP MODEL (TREE): " << endl;
        start = high_resolution_clock::now();
        models[1].current_state_num = models[1].current_state->get_state_num();
        models[1].traverseTree(0, horizon[i]);
        stop = high_resolution_clock::now();
        duration = duration_cast<microseconds>(stop - start);
        //cout << "Total Reward after " << horizon[i] << " steps long horizon: " << model.total_reward << ". Total duration: "<< duration.count() * 0.000001<< " seconds. Maximum memory used: " << model.max_memory_used/1024.0 << " MB"<< endl;
        cout << horizon[i] << "," << models[1].total_reward << "," << duration.count() * 0.000001 << "," << models[1].max_memory_used/1000.0 << endl;
        total_rewards_results[1][i] += models[1].total_reward;
        cout << endl;

        //cout << "NUMBER OF VMS: " << models[2].scenario->measurements["number_of_VMs"] << endl;

        cout << "FINITE MDP MODEL (CLASSIC): " << endl;
        start = high_resolution_clock::now();
        models[2].simpleEvaluation(horizon[i]);
        stop = high_resolution_clock::now();
        duration = duration_cast<microseconds>(stop - start);
        //cout << "Total Reward after " << horizon[i] << " steps long horizon: " << model.total_reward << ". Total duration: "<< duration.count() * 0.000001<< " seconds. Maximum memory used: " << model.max_memory_used/1024.0 << " MB"<< endl;
        cout << horizon[i] << "," << models[2].total_reward << "," << duration.count() * 0.000001 << "," << models[2].max_memory_used/1000.0 << endl;
        total_rewards_results[2][i] += models[2].total_reward;
        cout << endl;
    }
    }



    cout << endl;
        for(int j=0;j<11;j++){
            cout << total_rewards_results[0][j] / num_tests*1.0 << " , " << total_rewards_results[1][j] / num_tests*1.0 << " , " << total_rewards_results[2][j] / num_tests*1.0 << endl;
        }
    /*cout << '\n';
    cout << "Total results after running " << horizon.size() << " tests" << endl;
    cout << "Average total rewards : " << avg(total_rewards_results) << endl;
    cout << '\n';*/
    //cout << duration.count() << " microseconds" << endl;
}