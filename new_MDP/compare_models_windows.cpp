#include <iostream>
#include "FiniteMDPModel.h"
#include "ModelConf.h"
#include <vector>
#include "Complex.h"
#include <chrono>
#include <Windows.h>
#include <psapi.h>

#include "stdlib.h"
#include "stdio.h"
#include "string.h"

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

int main()
{
    int num_tests = 2;
    int training_steps = 10000;
    /*vector<int> horizon{1, 5, 10, 25, 50, 75, 100, 250, 500, 750, 1000, 2500, 5000, 7500 ,10000, 25000, 50000, 75000, 100000 };*/
    vector<int> horizon{100, 500, 1000};
    int seed = 21;
    //vector<int> horizon{100, 250, 500};
    int max_memory_used = 0;
    int load_period = 250;
    int MIN_VMS = 1;
    int MAX_VMS = 20;
    float epsilon = 0.7;
    string CONF_FILE = "mdp_small.json";
    ModelConf conf(CONF_FILE);
    float total_rewards_results[5][11];
    for (int i=0; i<5; i++){
        for (int j=0; j<11; j++){
            total_rewards_results[i][j] = 0.0;
        }
    }

    ComplexScenario scenario(5000, load_period, 10, MIN_VMS, MAX_VMS);

    pair<string, int> action;
    for (int number_of_tests =0; number_of_tests < num_tests; number_of_tests++){
    
    FiniteMDPModel model(conf.get_model_conf(), number_of_tests + 200);
    model.set_state(scenario.get_current_measurements());
    float total_reward = 0.0;
    
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

    for (int test_number = 0; test_number < num_tests; test_number++){
        for (int i = 0; i < horizon.size(); i++){

        //INFINITE MDP MODEL
        cout << "INFINITE MDP: " << endl;
        auto start = high_resolution_clock::now();
        model.resetValueFunction();
        model.value_iteration(0.1, false);
        for (int time = training_steps; time < training_steps + horizon[i]; time++){
            if (time == training_steps) 
                model.expected_reward = model.states[model.initial_state_num].value;
            model.takeAction(true);
            if (getValue() > model.max_memory_used)
                model.max_memory_used = getValue();
        }
        auto stop = high_resolution_clock::now();
        auto duration = duration_cast<microseconds>(stop - start);

        cout << "Horizon size: " << horizon[i] << endl;
        cout << "Total Reward Expected: " << model.expected_reward << endl;
        cout << "Total Reward Collected: " << model.total_reward << endl;
        cout << "Execution time (sec): " << duration.count() * 0.000001 << endl;
        cout << "Peak memory used (MB): " << model.max_memory_used / 1000000.0 << endl;

        cout << endl;

        model.resetModel();
        
        cout << "FINITE MDP MODEL (TREE): " << endl;
        start = high_resolution_clock::now();
        model.traverseTree(1, horizon[i]);
        stop = high_resolution_clock::now();
        duration = duration_cast<microseconds>(stop - start);

        cout << "Horizon size: " << horizon[i] << endl;
        cout << "Total Reward Expected: " << model.expected_reward << endl;
        cout << "Total Reward Collected: " << model.total_reward << endl;
        cout << "Execution time (sec): " << duration.count() * 0.000001 << endl;
        cout << "Peak memory used (MB): " << model.max_memory_used / 1000000.0 << endl;

        cout << endl;
        
        model.resetModel();
        
        cout << "FINITE MDP MODEL (CLASSIC): " << endl;
        start = high_resolution_clock::now();
        model.simpleEvaluation(horizon[i]);
        stop = high_resolution_clock::now();
        duration = duration_cast<microseconds>(stop - start);

        cout << "Horizon size: " << horizon[i] << endl;
        cout << "Total Reward Expected: " << model.expected_reward << endl;
        cout << "Total Reward Collected: " << model.total_reward << endl;
        cout << "Execution time (sec): " << duration.count() * 0.000001 << endl;
        cout << "Peak memory used (MB): " << model.max_memory_used / 1000000.0 << endl;

        cout << endl;

        model.resetModel();

        cout << "FINITE MDP MODEL (SQUARE ROOT): " << endl;
        start = high_resolution_clock::now();
        model.rootEvaluation(horizon[i]);
        stop = high_resolution_clock::now();
        duration = duration_cast<microseconds>(stop - start);
        
        cout << "Horizon size: " << horizon[i] << endl;
        cout << "Total Reward Expected: " << model.expected_reward << endl;
        cout << "Total Reward Collected: " << model.total_reward << endl;
        cout << "Execution time (sec): " << duration.count() * 0.000001 << endl;
        cout << "Peak memory used (MB): " << model.max_memory_used / 1000000.0 << endl;

        cout << endl;

        model.resetModel();
        }
    }
    }
}