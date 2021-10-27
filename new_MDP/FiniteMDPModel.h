#include <iostream>
#include <vector>
#include <map>
#include <nlohmann\json.hpp>
#include <stdexcept>
#include <string>
#include <stack>
#include <random>
#include <math.h>
#include <array>
#include "MDPModel.h"
#include "Complex.h"

#include "stdlib.h"
#include "stdio.h"
#include "string.h"

#include <Windows.h>
#include <psapi.h>

int parseLine(char* line){
    // This assumes that a digit will be found and the line ends in " Kb".
    int i = strlen(line);
    const char* p = line;
    while (*p <'0' || *p > '9') p++;
    line[i-3] = '\0';
    i = atoi(p);
    return i;
}

SIZE_T getValue(){ //Note: this value is in KB!
    /*FILE* file = fopen("/proc/self/status", "r");
    int result = -1;
    char line[128];

    while (fgets(line, 128, file) != NULL){
        if (strncmp(line, "VmRSS:", 6) == 0){
            result = parseLine(line);
            break;
        }
    }
    fclose(file);
    return result;*/
    PROCESS_MEMORY_COUNTERS_EX pmc;
    GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc));
    //SIZE_T virtualMemUsedByMe = pmc.PrivateUsage;
    SIZE_T physicalMemUsedByMe = pmc.WorkingSetSize;
    //return virtualMemUsedByMe;
    return physicalMemUsedByMe;
}

//std::random_device rd;
//std::seed_seq seed{rd()};
//std::default_random_engine eng(seed);


// for convenience
using json = nlohmann::json;

using namespace std;

class FiniteMDPModel: public MDPModel{
    public:
        stack<int> index_stack;
        stack<vector<pair<int , float>>> finite_stack;
        float total_reward = 0.0;
        int max_memory_used = 0;
        int steps_made = 0;
        float expected_reward = 0.0;
        default_random_engine eng;
        uniform_real_distribution<float> unif;
        int stack_memory = 0;

    FiniteMDPModel(json conf = json({}), int seed = 21){
        if (conf.contains("discount"))
        discount = conf["discount"];

        if (conf.contains("parameters"))
        parameters = _get_params(conf["parameters"]);

        for (auto& element : parameters.items()) {
            index_params.push_back(element.key());
            _update_states(element.key(), element.value());
        }


        int num_states = states.size();

        for (auto & element : states){
            element.set_num_states(num_states);
        }
        if (conf.contains("actions")){
        _set_maxima_minima(parameters, conf["actions"]);

        if (conf.contains("initial_qvalues"))
            _add_qstates(conf["actions"], conf["initial_qvalues"]);
        }
        update_algorithm  = true;
        
        eng = default_random_engine(seed);
        unif = uniform_real_distribution<float>(0,1);
            
    };

    void setInitialState(State &s){
        current_state_num = s.state_num;
    }

    /*void _q_update_finite(QState &qstate, vector<State> &V){
        float new_qvalue = 0.0;
        float r;
        float t;
        for (int i=0; i < V.size(); i++){
            t = qstate.get_transition(i);
            r = qstate.get_reward(i);
            new_qvalue += t * (r +V[i].get_value());
        }
        qstate.set_qvalue(new_qvalue);
    }*/

    void _q_update_finite(QState &qstate, vector<pair<int,float>> V){
        float new_qvalue = 0.0;
        float r;
        float t;
        for (int i=0; i < V.size(); i++){
            t = qstate.get_transition(i);
            r = qstate.get_reward(i);
            new_qvalue += t * (r +V[i].second);
        }
        qstate.set_qvalue(new_qvalue);
    }


    vector<pair<int, float>> calculateValues(int k, int starting_index, vector<pair<int, float>> V, bool tree = false){
        vector<pair<int,float>> V_tmp = V;
        for (int i = starting_index+1 ; i < k+1; i++){
            for (int j = 0 ; j < states.size(); j++ ){
                for (int m = 0; m < states[j].get_qstates().size(); m++){
                    _q_update_finite(states[j].qstates[m], V_tmp);
                }
                states[j].update_value();
            }
            V_tmp = getStateValues(states);
            if (!tree){
                index_stack.push(i);
                finite_stack.push(V_tmp);

                stack_memory += V_tmp.size() * sizeof(float);
            }
        }
        if (getValue() > max_memory_used){
            max_memory_used = getValue();
        }
        return V_tmp;

    }

    pair<string,int> finite_suggest_action(){
        return states[current_state_num].get_optimal_action();
    }

    void takeAction(bool isInfinite, bool p=false){
        pair<string,int> action;
        if (isInfinite) {action = suggest_action();}
        else {action = finite_suggest_action();}
        float reward;
        //float reward = scenario.execute_action(action);
        //json meas = scenario.get_current_measurements();

        int prev_state_num = current_state_num;
        //current_state_num = _get_state(meas)->get_state_num();
        float x = unif(eng);
        float acc = 0.0;
        for (int i=0; i< states[prev_state_num].qstates.size();i++){
            if (states[prev_state_num].qstates[i].action.first == action.first){
                for (int j=0; j<states[prev_state_num].qstates[i].transitions.size();j++){
                    if (states[prev_state_num].qstates[i].get_transition(j) > 0){
                        acc += states[prev_state_num].qstates[i].get_transition(j);
                        if (x < acc){
                            current_state_num = j;
                            reward = states[prev_state_num].qstates[i].get_reward(current_state_num);
                            break;
                        }
                    }
                }
            }
        }

        total_reward += reward;
        if (p){
            cout << "Current state: " << prev_state_num<< endl;
            for (int i=0; i < states[prev_state_num].qstates.size(); i++){
                cout << "   Qstate: " << states[prev_state_num].qstates[i].action.first << " ,QValue:" << states[prev_state_num].qstates[i].qvalue << endl;
            }
            cout << action.first << " ,Reward: " << reward << "   Next state: " << endl << states[current_state_num] << endl;
        }
    }

    void traverseTree(int l, int r){
        if (r >=l ){

            int k = l + (r-l)/2;//index which needs to be calculated

            steps_made++;

            //vector<State> V;
            vector<pair<int,float>> V;
            if (finite_stack.empty()){
                resetValueFunction();
                //V = calculateValues(k, 0, states, true); //if no vector is saved in memory, calculate objective from the beginning
                V = calculateValues(k, 0, getStateValues(states), true);

            }
            else{

                V = calculateValues(k, index_stack.top(), finite_stack.top(), true);//use last saved vector in memory to calculate objective
            }

            finite_stack.push(V);//add newly calculated vector to memory
            index_stack.push(k);

            stack_memory += V.size() * sizeof(float);

            if (V[initial_state_num].second > expected_reward) expected_reward = V[initial_state_num].second;

            traverseTree(k+1, r);

            //states = V;
            loadValueFunction(V);
            takeAction(false);

            finite_stack.pop();//remove top of the stack from memory
            index_stack.pop();

            stack_memory -= V.size() * sizeof(float);

            traverseTree(l, k-1);
        }
    }

    void simpleEvaluation(int horizon){
            //vector<State> V;
            vector<pair<int,float>> V;
            resetValueFunction();
            //V = calculateValues(horizon, 0, states);
            V = calculateValues(horizon, 0, getStateValues(states));
            expected_reward = V[initial_state_num].second;

            while (!finite_stack.empty()){

                steps_made++;

                //states = finite_stack.top();
                loadValueFunction(finite_stack.top());
                takeAction(false);

                finite_stack.pop();
                index_stack.pop();
                stack_memory -= V.size() * sizeof(float);
            }
        }


    void naiveEvaluation(int horizon){

        vector<pair<int,float>> V;
        int steps_remaining = horizon;
        resetValueFunction();

        V = calculateValues(steps_remaining, 0, getStateValues(states),true);
        expected_reward = V[initial_state_num].second;

        while(steps_remaining > 0){
            V = calculateValues(steps_remaining, 0, getStateValues(states),true);
            loadValueFunction(V);
            takeAction(false);
            steps_made++;
            steps_remaining--;
        }
    }

    void rootEvaluation(int horizon){

        vector<pair<int,float>> V;
        int steps_remaining = horizon;
        resetValueFunction();

        V = calculateValues(1, 0, getStateValues(states),true);

        int floor_of_square_root = floor(sqrt(horizon));

        for (int i = 2; i <= horizon; i++){
            V = calculateValues(i, i-1, V, true);
            if (i % floor_of_square_root == 0){
                finite_stack.push(V);
                index_stack.push(i);

                stack_memory += states.size()*sizeof(float);
            }
        }

        expected_reward = V[initial_state_num].second;

        while(steps_remaining > 0){
            if (finite_stack.empty()){
                V = calculateValues(steps_remaining, 0, getStateValues(states));
            }
            else{
                if(index_stack.top() == steps_remaining){
                    V = finite_stack.top();
                }
                else{
                    V = calculateValues(steps_remaining, index_stack.top(), finite_stack.top());
                }
            }

            loadValueFunction(V);
            takeAction(false);

            finite_stack.pop();
            index_stack.pop();

            stack_memory -= states.size()*sizeof(float);

            steps_made++;
            steps_remaining--;
        }
    }


/*
    void printValueFunction(){
        if(!finite_stack.empty()){
            for (int i = 0; i < finite_stack.top().size(); i++){
                cout << "State " << finite_stack.top()[i].get_state_num()<< ": " << finite_stack.top()[i].value << endl;
            }
        }
        else{
            for (int i = 0; i < states.size(); i++){
                cout << "State " << states[i].get_state_num()<< ": " << states[i].value << endl;
            }  
        }
    }
*/
    void resetValueFunction(){
        for (int i=0; i<states.size(); i++){
            for (int j=0; j < states[i].qstates.size(); j++){
                states[i].qstates[j].qvalue = 0.0;
            }
            states[i].value = 0.0;
        }
    }


    void resetModel(){
        resetValueFunction();
        current_state_num = initial_state_num;
        total_reward = 0.0;
        expected_reward = 0.0;
        max_memory_used = 0.0;
        steps_made = 0;
        stack_memory = 0;
    }



};
