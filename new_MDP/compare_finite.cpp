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
    //float r = myRand(0, 1);
    double r = unif(eng);
    for (int i = 1; i < n + 1; i++)
    {
        if (r < x * i)
            return v[i - 1];
    }
    return v[0];
}

int main()
{
    int num_tests = 1;
    int training_steps = 10000;
    /*vector<int> horizon{1, 5, 10, 25, 50, 75, 100, 250, 500, 750, 1000, 2500, 5000, 7500 ,10000, 25000, 50000, 75000, 100000 };*/
    vector<int> horizon{100, 250, 500, 750, 1000, 2500, 5000, 7500};
    //vector<int> horizon{500};
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

    ComplexScenario scenario(5000, load_period, 10, MIN_VMS, MAX_VMS);
    FiniteMDPModel model(conf.get_model_conf());
    model.set_state(scenario.get_current_measurements());
    float total_reward = 0.0;
    pair<string, int> action;
    //TRAIN THE MODEL

    for (int time = 0; time < training_steps; time++){

        double x = unif(eng);
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
            model.value_iteration(0.1);
        }
    }
    model.initial_state_num = model.current_state_num;

    for (int test_number = 0; test_number < num_tests; test_number++){
    for (int i = 0; i < horizon.size(); i++)
    {


        //INFINITE MDP MODEL
        cout << "INFINITE MDP: " << endl;

        auto start = high_resolution_clock::now();

        model.resetValueFunction();
        model.value_iteration(0.1, false);

        for (int time = training_steps; time < training_steps + horizon[i] ; time++)
        {

            model.takeAction(true);

            if (getValue2() > max_memory_used) max_memory_used = getValue2();
        }

        auto stop = high_resolution_clock::now();
        auto duration = duration_cast<microseconds>(stop - start);

        cout << horizon[i] << "," << model.total_reward << "," << duration.count() * 0.000001 << "," << max_memory_used/1000.0 << endl;
        
        total_rewards_results[0][i] += model.total_reward;

        cout << endl;
        
        max_memory_used = 0;

        model.resetModel();
        
        cout << "FINITE MDP MODEL (TREE): " << endl;
        start = high_resolution_clock::now();
        model.traverseTree(1, horizon[i]);
        stop = high_resolution_clock::now();
        duration = duration_cast<microseconds>(stop - start);
        cout << horizon[i] << "," << model.total_reward << "," << duration.count() * 0.000001 << "," << model.max_memory_used/1000.0 << endl;
        total_rewards_results[1][i] += model.total_reward;
        cout << "Steps made: " << model.steps_made << endl;
        cout << endl;
        //delete models[1].finite_stack;
        
        model.resetModel();

        /*
        cout << "FINITE MDP MODEL (CLASSIC): " << endl;
        start = high_resolution_clock::now();
        model.simpleEvaluation(horizon[i]);
        stop = high_resolution_clock::now();
        duration = duration_cast<microseconds>(stop - start);
        cout << horizon[i] << "," << model.total_reward << "," << duration.count() * 0.000001 << "," << model.max_memory_used/1000.0 << endl;
        total_rewards_results[2][i] += model.total_reward;
        cout << "Steps made: " << model.steps_made << endl;
        cout << endl;

        model.resetModel();*/
    }
    }



    cout << endl;
        for(int j=0;j<horizon.size();j++){
            cout << horizon[j] << " , "<< total_rewards_results[0][j] / num_tests*1.0 << " , " << total_rewards_results[1][j] / num_tests*1.0 << " , " << total_rewards_results[2][j] / num_tests*1.0 << endl;
            /*if ((total_rewards_results[0][j] / num_tests*1.0 - total_rewards_results[2][j] / num_tests*1.0) < 0.00001 ){
                main();
            }
            else{
                models[1].printDetails();
            }*/
        }
}