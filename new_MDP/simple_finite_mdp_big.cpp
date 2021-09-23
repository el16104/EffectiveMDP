#include <iostream>
//#include "MDPModel.h"
#include "ModelConf.h"
#include <vector>
#include "Complex.h"
#include <chrono>
#include "FiniteMDPModel.h"
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
    int num_tests = 18;
    vector<int> horizon{1, 5, 10, 25, 50, 75, 100, 250, 500, 750, 1000, 2500, 5000, 7500 ,10000, 25000, 50000, 100000 };
    int training_steps = 5000;
    int eval_steps = 1;
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
        FiniteMDPModel model(scenario, conf.get_model_conf());
        model.set_state(scenario.get_current_measurements());
        float total_reward = 0.0;
        for (int time = 0; time < training_steps + eval_steps; time++)
        {
            if (time < training_steps){

                float x = myRand(0, 1);
                if (x < epsilon){
                    action = randomchoice(model.get_legal_actions());
                }
                else{
                    action = model.suggest_action();
                }

                float reward = scenario.execute_action(action);
                json meas = scenario.get_current_measurements();
                model.update(action, meas, reward);

                if (time % 500 == 1){
                    model.value_iteration(0.1);
                }
            }
            else{
                auto start = high_resolution_clock::now();
                try{model.simpleEvaluation(horizon[i]);}
                catch(...){cout << "TOO MUCH MEMORY" << endl;}
                auto stop = high_resolution_clock::now();
                auto duration = duration_cast<microseconds>(stop - start);
                //cout << "Total Reward after " << horizon[i] << " steps long horizon: " << model.total_reward << ". Total duration: "<< duration.count() * 0.000001<< " seconds. Maximum memory used: " << model.max_memory_used/1024.0 << " MB"<< endl;
                cout << horizon[i] << "," << model.total_reward << "," << duration.count() * 0.000001 << "," << model.max_memory_used/1000.0 << endl;
            }

            //cout << "Reward: " << total_reward << endl;
        }
        total_rewards_results.push_back(model.total_reward);
    }
    cout << '\n';
    cout << "Total results after running " << 7 << " tests" << endl;
    cout << "Average total rewards : " << avg(total_rewards_results) << endl;
    cout << '\n';
}