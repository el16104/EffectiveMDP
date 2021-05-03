#include <iostream>
#include <vector>
#include <map>

using namespace std;

class State{
public: 
    vector<QState> qstates;
    int state_num;
    int num_states;
    float value = 0.0;
    QState* best_qstate;
    int num_visited = 0;

    State(int state_num = 0, float initial_value = 0, int num_states = 0){
        vector<QState> qstates;
        int state_num   = state_num;
        int num_states  = num_states;
        float value = 0.0;
        QState* best_qstate;
        int num_visited = 0;
    }

    void visit(){
        num_visited++;
    }

    int get_state_num(){
        return state_num;
    }

    void set_num_states(int num_states){
        num_states = num_states;
    }

    float get_value(){
        return value;
    }

    QState* get_best_qstate(self){
        return *best_qstate;
    }

    void update_value(){

        best_qstate = qstates[0];
        value = qstates[0].get_qvalue();

        for (auto & element : qstates) {
            if (element.get_qvalue() > value){
                best_qstate = element;
                value = element.get_value();
            }
        }
    }

    void add_qstate(QState q){
        qstates.push_back(q);
        if (best_qstate == NULL)
            best_qstate = &q;
    }

    vector<QState> get_qstates(){
        return qstates;
    }


    QState get_qstate(action){

        for (auto & element : qstates){
            if (element.get_action() == action)
                return element;
        }
    }

    int* get_max_transitions(){
        int* trans = new int[num_states];
        for (int i=0; i < num_states; i++){
            for (auto & element : qstates){
                if element.has_transition(i){
                    if i in transitions
                        trans[i] = max(trans[i], element.get_transition(i))
                    else
                        trans[i] = element.get_transition(i)
                }
            }
        }
    }

    vector<pair<string,string>> get_legal_actions(){

        vector<pair<string,string>> actions;
        for (auto & element : qstates){
            actions.push_back(element.get_action())
        }
        return actions;
    }


};