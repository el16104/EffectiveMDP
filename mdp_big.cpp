#include <iostream>
#include "MDPModel.h"
#include "ModelConf.h"
#include <vector>
#include "Complex.h"


using namespace std;

double avg ( vector<double> v ){
        double return_value = 0.0;
        int n = v.size();
       
        for ( int i=0; i < n; i++)
        {
            return_value += v[i];
        }
       
        return ( return_value / n);
}

pair<string,int> randomchoice(vector<pair<string,int>> v){
    double n = (double)v.size();
    double x = 1.0/n;
    double r = myRand(0,1);
    for ( int i = 1; i < n+1 ; i++){
        if (r < x*i)
            return v[i-1];
    }
}

int main(){
    int num_tests = 20;
    int training_steps = 5000;
    int eval_steps = 2000;
    int load_period = 250;
    int MIN_VMS = 1;
    int MAX_VMS = 20;
    double epsilon = 0.7;
    string CONF_FILE = "/home/giannispapag/Thesis_Code/EffectiveMDP/Sim_Data/mdp_big.json";
    ModelConf conf(CONF_FILE);
    vector<double> total_rewards_results;

    pair<string,int> action;

    for (int i=0; i < num_tests ; i++){
        ComplexScenario scenario(training_steps, load_period, 10, MIN_VMS, MAX_VMS);
        MDPModel model(conf.get_model_conf());
        model.set_state(scenario.get_current_measurements());

        double total_reward = 0.0;

        for (int time = 0 ; time < training_steps + eval_steps; time++){
            double x = myRand(0,1);
            if (x < epsilon && time < training_steps){
                action = randomchoice(model.get_legal_actions());
                cout << "Action chosen (RANDOMLY): " << action.first << " " << action.second << endl;
            }
            else{            
                action = model.suggest_action();         
                cout << "Action suggested by MODEL: " << action.first << " " << action.second << endl; 
            }


            double reward = scenario.execute_action(action);
            json meas = scenario.get_current_measurements();
            model.update(action, meas, reward);

            if (time % 500 == 1)
                model.value_iteration(0.1);

            if (time > training_steps)
                total_reward += reward;

            //cout << "Incoming Load :" << scenario.get_incoming_load() << ", Current Capacity: " << scenario.get_current_capacity() << endl;
            }

        cout << "Reward: " << total_reward << endl;
        total_rewards_results.push_back(total_reward);
    }
    cout << '\n';
    cout <<  "Total results after running " << num_tests << " tests" << endl;
    cout << "Average total rewards : " <<  avg(total_rewards_results) << endl;
    cout << '\n';

}