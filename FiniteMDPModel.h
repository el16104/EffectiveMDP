#include <iostream>
#include <vector>
#include <map>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>
#include <stack>
#include "MDPModel.h"
#include "Complex.h"

// for convenience
using json = nlohmann::json;

using namespace std;

class FiniteMDPModel: public MDPModel{
    stack<pair<int, vector<State>>> finite_stack;
    int current_state_num;
    ComplexScenario* scenario;

    void setInitialState(State &s){
        current_state_num = s.state_num;
    }

    void setScenario(ComplexScenario &s){
        scenario = &s;
    }

    void _q_update_finite(QState &qstate, vector<State> &V){
        double new_qvalue = 0;
        double r;
        double t;
        for (int i=0; i < states.size(); i++){
            t = qstate.get_transition(i);
            r = qstate.get_reward(i);
            new_qvalue += t * (r + V[i].get_value());
        }
        qstate.set_qvalue(new_qvalue);
    }

    vector<State> calculateValues(int k, int starting_index, vector<State> &V){
        vector<State> V_tmp = V;
        vector<State> V_aux;
        for (int i = starting_index+1 ; i < k+1; i++){
            V_aux = {};
            for (auto& s:states){
                for (auto& qs:s.get_qstates()){
                    _q_update_finite(qs, V_tmp);
                }
                s.update_value();
            }
            V_tmp = V_aux;
        }
        return V_tmp;

    }

    pair<string,int> finite_suggest_action(vector<State> &V){
        return V[current_state_num].get_optimal_action();
    }

    void traverseTree(int r, int l){
        if (r >=l){
            int k = l + (r-1)/2;
            vector<State> V;
            if (finite_stack.empty()){
                vector<State> V_temp = {};
                V = calculateValues(k, 0, V_temp);
            }
            else{
                V = calculateValues(k, finite_stack.top().first, finite_stack.top().second);
            }
            finite_stack.push(make_pair(k, V));
            traverseTree(k+1, r);
            pair<string,int> action = this->finite_suggest_action(V);
            double reward = scenario->execute_action(action);
            json meas = scenario->get_current_measurements();
            //model.update(action, meas, reward);
            finite_stack.pop();
            traverseTree(l, k-1);
        }
    }

};
