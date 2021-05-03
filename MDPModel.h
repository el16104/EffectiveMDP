#include <iostream>
#include <vector>
#include <map>

using namespace std;

int max(int a, int b){
    if (a>=b) return a;
    else return b;
}

class State;

class QState{
public:
    pair<string,string> action;
    int num_taken;
    float qvalue;
    int* transitions;
    float* rewards;
    int num_states;

    QState(pair<string, string> action, int num_states, float qvalue = 0.0){
        action = action;
        num_taken = 0;
        qvalue = qvalue;
        num_states = num_states;
        transitions = new int[num_states];
        rewards = new float[num_states];
        for (int i = 0; i < num_states; i++) transitions[i] = 0;
        for (int i = 0; i < num_states; i++) rewards[i] = 0;
    }

    void update(State* new_state, float reward);


    pair<string, string> get_action(){
        return action;
    }

    float get_qvalue(){
        return qvalue;
    }

    bool has_transition(int state_num){
        return (transitions[state_num] > 0);
    }


    float get_transition(int state_num){
        if (num_taken == 0)
            return (1 /num_states);
        else
            return (transitions[state_num] /num_taken);
    }

    int get_num_transitions(int state_num){
        return transitions[state_num];
    }


    float get_reward(int state_num){
        if (transitions[state_num] == 0)
            return 0.0;
        else
            return (rewards[state_num] / transitions[state_num]);
    }

    void set_qvalue(float qvalue){
        qvalue = qvalue;
    }

    int get_num_taken(){
        return num_taken;
    }


    int* get_transitions(){
        return transitions;
    }

    float* get_rewards(){
        return rewards;
    }
};

class State{
public: 
    vector<QState> qstates;
    int state_num;
    int num_states;
    float value = 0.0;
    QState* best_qstate;
    int num_visited = 0;

    State(int state_num = 0, float initial_value = 0, int num_states = 0){
        //vector<QState> qstates;
        state_num   = state_num;
        num_states  = num_states;
        value = initial_value;
        //QState* best_qstate;
        //int num_visited = 0;
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

    QState* get_best_qstate(){
        return best_qstate;
    }

    void update_value(){

        best_qstate = &qstates[0];
        value = qstates[0].get_qvalue();

        for (auto & element : qstates) {
            if (element.get_qvalue() > value){
                best_qstate = &element;
                value = element.get_qvalue();
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


    QState get_qstate(pair<string, string> action){

        for (auto & element : qstates){
            if (element.get_action() == action)
                return element;
        }
    }

   map<int,int> get_max_transitions(){
        map<int,int> trans;
        
        for (int i=0; i < num_states; i++){
            for (auto & element : qstates){
                if (element.has_transition(i)){
                    if (trans.find(i) != trans.end())
                        trans.insert(pair<int,int>(i, max(trans.at(i), element.get_transition(i))));
                    else
                        trans.insert(pair<int,int>(i, element.get_transition(i)));
                }
            }
        }
        return trans;
    }

    vector<pair<string,string>> get_legal_actions(){

        vector<pair<string,string>> actions;
        for (auto & element : qstates){
            actions.push_back(element.get_action());
        }
        return actions;
    }


};

void QState::update(State* new_state, float reward){
        num_taken++;
        int state_num = new_state->get_state_num();
        transitions[state_num] += 1;
        rewards[state_num] += reward;
    }

class MDPModel{
    public:
        float discount;
        State* states;
        pair<string, float>* index_params;
        //self.index_states  = list(self.states);
        State current_state;
        float update_error  = 0.01;
        int max_updates   = 100;
};
