#include <iostream>
#include <vector>
#include <map>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>
#include <stack>
#include "MDPModel.h"
#include "Complex.h"

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

// for convenience
using json = nlohmann::json;

using namespace std;

class FiniteMDPModel: public MDPModel{
    public:
        stack<int> index_stack;
        stack<vector<State>> finite_stack;
        int current_state_num;
        ComplexScenario* scenario;
        float total_reward = 0.0;
        int max_memory_used = 0;

    FiniteMDPModel(ComplexScenario &scen,json conf = json({})){
        scenario = &scen;
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
            
    };

    void setInitialState(State &s){
        current_state_num = s.state_num;
    }

    void setScenario(ComplexScenario &s){
        scenario = &s;
    }

    void _q_update_finite(QState &qstate, vector<State> &V){
        float new_qvalue = 0;
        float r;
        float t;
        for (int i=0; i < V.size(); i++){
            t = qstate.get_transition(i);
            r = qstate.get_reward(i);
            new_qvalue += t * (r + V[i].get_value());
        }
        qstate.set_qvalue(new_qvalue);
    }

    vector<State> calculateValues(int k, int starting_index, vector<State> &V, bool tree = false){
        vector<State> V_tmp = V;
        for (int i = starting_index+1 ; i < k+1; i++){
            for (auto& s:states){
                for (auto& qs:s.get_qstates()){
                    _q_update_finite(qs, V_tmp);
                }
                s.update_value();
            }
            if (!tree){
                index_stack.push(i);
                finite_stack.push(V_tmp);
            }
            V_tmp = states;
        }
        if (getValue() > max_memory_used){
            max_memory_used = getValue();
        }
        return V_tmp;

    }

    pair<string,int> finite_suggest_action(){
        return states[current_state_num].get_optimal_action();
    }

    void finite_update(pair<string,int> action, json measurements, float reward){
        current_state->visit();
        QState* qstate = current_state->get_qstate(action);
        if (qstate->num_states == -1) return;
        State* new_state = _get_state(measurements);
        qstate->update(new_state->get_state_num(), reward);
        if (update_algorithm){
            _q_update(*qstate);
            current_state->update_value();
        }
        else{
            value_iteration();
        }
        current_state = new_state;
    }

    void traverseTree(int l, int r){
        if (r >=l){
            int k = l + (r-l)/2;
            vector<State> V;
            if (finite_stack.empty()){
                V = calculateValues(k, 0, states, true);
            }
            else{
                V = calculateValues(k, index_stack.top(), finite_stack.top(), true);
            }
            finite_stack.push(V);
            index_stack.push(k);
            this->traverseTree(k+1, r);
            this->states = V;
            pair<string,int> action = this->finite_suggest_action();
            float reward = scenario->execute_action(action);
            total_reward += reward;
            json meas = scenario->get_current_measurements();
            current_state_num = _get_state(meas)->get_state_num();
            //model.(action, meas, reward);
            finite_stack.pop();
            index_stack.pop();
            this->traverseTree(l, k-1);
        }
    }

        void simpleEvaluation(int horizon){
            vector<State> V;
            current_state_num = current_state->get_state_num();
            V = calculateValues(horizon, 0, states);
            while (!finite_stack.empty()){
                this->states = finite_stack.top();
                pair<string,int> action = this->finite_suggest_action();
                float reward = scenario->execute_action(action);
                total_reward += reward;
                json meas = scenario->get_current_measurements();
                current_state_num = _get_state(meas)->get_state_num();
                finite_stack.pop();
                index_stack.pop();
            }
        }



};
