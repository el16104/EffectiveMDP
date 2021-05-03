#include <iostream>
#include <vector>
#include <map>

using namespace std;

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

    void update(State new_state, float reward){
        num_taken++;
        int state_num = new_state.get_state_num();
        transitions[state_num] += 1;
        rewards[state_num] += reward;
    }

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