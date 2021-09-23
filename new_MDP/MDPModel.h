#include <iostream>
#include <vector>
#include <map>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>
#include <stack>

// for convenience
using json = nlohmann::json;

using namespace std;

string printableParameters(map<string,pair<float,float>> params){
    string s = "[";
    for (auto& element:params){
        s = s + "(";
        s = s + element.first + "," + to_string(element.second.first) + "," + to_string(element.second.second) + ") ";
    }
    s = s + "]";
    return s;
}

//class State;

class QState{
public:
    pair<string,int> action;
    int num_taken;
    float qvalue;
    vector<int> transitions={};
    vector<float> rewards = {};
    int num_states;

    float upper_bound;//Maximum value of QStates for a given state
    float lower_bound;//Minimum value of QStates for a given state

    QState(pair<string,int> actionn, int numstates, float qvaluee){
        action = actionn;
        num_taken = 0;
        qvalue = qvaluee;
        num_states = numstates;
        if (num_states > 0){
            for (int i=0; i< num_states;i++){
            transitions.push_back(0);
            rewards.push_back(0.0);
            }
        }

    }

    QState(){
        std::string s0 ("NOT SET");
        action.first = s0;
        action.second = -1;
        num_taken = 0;
        qvalue = 0.0;
        num_states = -1;
        if (num_states > 0){
            for (int i=0; i< num_states;i++){
            transitions.push_back(0);
            rewards.push_back(0.0);
            }
        }

    }
    void update(int state_num, float reward){
        num_taken++;
        transitions[state_num] += 1;
        rewards[state_num] += reward;
    }


    pair<string, int> get_action(){
        return action;
    }

    float get_qvalue(){
        return qvalue;
    }

    bool has_transition(int state_num){
        return (transitions[state_num] > 0);
    }


    float get_transition(int state_num){
        if (num_taken == 0){
            return (1.0 /(float)num_states);
        }
        else{
            return ((float)transitions[state_num] /(float)num_taken);
        }
    }

    int get_num_transitions(int state_num){
        return transitions[state_num];
    }


    float get_reward(int state_num){
        if (transitions[state_num] == 0)
            return 0.0;
        else
            return (rewards[state_num] / (float)transitions[state_num]);
    }

    void set_qvalue(float qvaluee){
        qvalue = qvaluee;
    }

    int get_num_taken(){
        return num_taken;
    }


    vector<int> get_transitions(){
        return transitions;
    }

    vector<float> get_rewards(){
        return rewards;
    }

    friend ostream &operator<<( ostream &output, const QState& q);

};

ostream &operator<<( ostream &output, const QState& q){ 
         output << "Action: "<< q.action.first<<"\tQ-value: "<< q.qvalue << "\tTaken: "<< q.num_taken << "\tUpper Bound: "<< q.upper_bound << endl;
         return output;            
}

class State{
public: 
    vector<QState> qstates ={};
    int state_num;
    int num_states;
    float value;
    QState* best_qstate = NULL;
    int num_visited;
    map<string,pair<float,float>> parameters;
    float max_lower_bound = -INFINITY;

    State(map<string,pair<float,float>> parameterss = {} , int statenum = 0, float initialvalue = 0.0, int numstates = 0){
        value = 0.0;
        num_visited = 0;
        state_num   = statenum;
        num_states  = numstates;
        value = initialvalue;
        parameters = parameterss;
        best_qstate = NULL;
        vector<QState*> qstates = {};
        max_lower_bound = -INFINITY;
    }


    void visit(){
        num_visited++;
    }

    int get_state_num(){
        return state_num;
    }

    void set_num_states(int numstates){
        num_states = numstates;
    }

    float get_value(){
        return value;
    }

    QState* get_best_qstate(){
        return best_qstate;
    }

    pair<string,int> get_optimal_action(){
        return best_qstate->get_action();
    }

    int best_action_num_taken(){
        return best_qstate->get_num_taken();
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

    map<string,pair<float,float>> get_parameters(){
        return parameters;
    } 

    void add_new_parameter(string name, pair<float,float> values){
        parameters[name] = values;
    }

    pair<bool,pair<float,float>> get_parameter(string param){
        for (auto& x:parameters){
            if (x.first == param)
                return make_pair(true, x.second);
        }
    }



    void add_qstate(QState &q){
        qstates.push_back(q);
        if (!best_qstate){
            best_qstate = &q;
        }
    }

    vector<QState> get_qstates(){
        return qstates;
    }


    QState* get_qstate(pair<string, int> action){

        for (auto & element : qstates){
            if (element.get_action() == action)
                return &element;
        }
    }

    vector<pair<string, int>> get_legal_actions(){

        vector<pair<string, int>> actions;
        for (auto & element : qstates){
            actions.push_back(element.get_action());
        }
        return actions;
    }

    friend ostream &operator<<(ostream &output, State& s);

    void print_detailed(){
        cout << state_num << ": " << printableParameters(parameters) << ", visited: "<< num_visited << "\tMax Lower Bounds: "<< max_lower_bound <<endl;
        for (auto& qs:this->get_qstates())
            cout << qs << endl;
    }


};

ostream &operator<<(ostream &output, State& s){
        output << s.state_num << ": " << printableParameters(s.parameters) <<endl;
        return output;
}



class MDPModel{
    public:
        float discount;
        vector<State> states = {State()};
        vector<string> index_params = {};
        State* current_state = NULL;
        json parameters = {};
        float update_error = 0.1;
        bool update_algorithm;
        int max_VMs;
        int min_VMs;
        
    MDPModel(json conf = json({}), bool upd_alg = true){
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
        update_algorithm  = upd_alg;
            
    };


    json _get_params(json par){
        json new_pars;
        for (auto& element : par.items()){
            new_pars[element.key()] = {};
            if (element.value().contains("values")){
                vector<pair<float,float>> values;
                for (auto& x:element.value()["values"]){
                    values.push_back(make_pair(x, x));
                }
                new_pars[element.key()]["values"] = values;
            }

            else if (element.value().contains("limits")){
                vector<pair<float,float>> values;
                vector<float> aux;
                for (auto& x:element.value()["limits"]) aux.push_back(x);
                for (int i = 1; i< aux.size(); i++ )
                    values.push_back(make_pair(aux[i-1] , aux[i]));
                new_pars[element.key()]["values"] = values;
            }
        }
        return new_pars;
    }

    void set_state(json measurements){
        current_state = _get_state(measurements);
    }

    void _update_states(string name, json new_parameter){
        int statenum = 0;
        vector<State> new_states ={};
        for (auto& x:new_parameter["values"]){
            for (auto& s:states){
                map<string,pair<float,float>> last;
                last = s.get_parameters();
                State new_state(last, statenum);
                new_state.add_new_parameter(name, x);
                new_states.push_back(new_state);
                statenum++;
            }
        }
        states = new_states;
    }

    State* _get_state(json measurements){
        for (auto& s:states){
            bool matches = true;
            for (auto& par:s.get_parameters()){
                float min_v = par.second.first;
                float max_v = par.second.second;
                if (measurements[par.first] < min_v || measurements[par.first] > max_v){
                    matches = false;
                    break;
                }
            }
            if (matches) return &s;
        }
    }

    void _set_maxima_minima(json parameters, json acts){
        if (acts.contains("add_VMs") || acts.contains("remove_VMs")){
            vector<int> vms;
            for (auto& x:parameters["number_of_VMs"]["values"]){
                vms.push_back(x[0]);
            }
            max_VMs = *max_element(vms.begin(), vms.end());
            min_VMs = *min_element(vms.begin(), vms.end());

        }
    }

    void _add_qstates(json acts, float initq){
        int num_states = states.size();
        for (auto& action:acts.items()){
            for (auto& val:action.value()){
                pair<string,int> act = make_pair(action.key(), val);
                for (auto& s:states){
                        if (_is_permissible(s, act)){
                            QState q(act, num_states, initq);
                            s.add_qstate(q);
                        }

                }
            }
        }

        for (auto& s:states){
            s.update_value();
        }
    }

    bool _is_permissible(State &s, pair<string,int> a){
        string action_type = a.first;
        int action_value = a.second;
        pair<bool, pair<float,float>> param_values;
        if (action_type == "add_VMs"){
             param_values = s.get_parameter("number_of_VMs");
             if (param_values.first)
                return (max(param_values.second.first , param_values.second.second) + action_value <= max_VMs);
        }
        else if (action_type == "remove_VMs"){
             param_values = s.get_parameter("number_of_VMs");
             if (param_values.first)
                return (min(param_values.second.first , param_values.second.second) - action_value >= min_VMs);
        }
        else return true;
    }

    pair<string,int> suggest_action(){
        return current_state->get_optimal_action();
    }

    vector<pair<string, int>> get_legal_actions(){
        return current_state->get_legal_actions();
    }
    
    void update(pair<string,int> action, json measurements, float reward){
        current_state->visit(); //increase number of times visited by 1 for the current state

        QState* qstate = current_state->get_qstate(action); //find qstate corresponding to the chosen action
        if (qstate->num_states == -1) return;

        State* new_state = _get_state(measurements); //find next state corresponding to current measurements
        
        qstate->update(new_state->get_state_num(), reward); //increase number of times taken, create transition between action and next state and assign reward
        
        if (update_algorithm){
            _q_update(*qstate); //set qvalue of chosen action to new qvalue
            current_state->update_value(); //update the value of the current state, choosing the maximum of its qvalues
        }

        else{
            this->value_iteration(0.1); //execute Value Iteration
        }

        current_state = new_state; //move the agent to the next state
    }


    //UPDATE_FINITE: moves the agent to the new state after choosing an action without updating the model
    void update_finite(pair<string,int> action, json measurements, float reward){
        State* new_state = _get_state(measurements);
        current_state = new_state;
    }

    void _q_update(QState &qstate){
        float new_qvalue = 0.0;
        float r;
        float t;
        for (int i=0; i < states.size(); i++){
            t = qstate.get_transition(i);
            r = qstate.get_reward(i);
            new_qvalue += t * (r + discount * states[i].get_value());
        }
        qstate.set_qvalue(new_qvalue);
    }

    void _v_update(State &state){
        for (int i =0; i < state.qstates.size(); i++)
        {
            _q_update(state.qstates[i]);
        }
        state.update_value();
    }

    void value_iteration(float error = -1){
        if (error < 0)
            error = update_error;
        bool repeat = true;
        float old_value;
        float new_value;
        while(repeat){
            //update_bounds();//calculate upper and lower bounds
            repeat = false;
            for (int i=0; i < states.size(); i++){
                old_value = states[i].get_value();
                _v_update(states[i]);
                new_value = states[i].get_value();
                if (abs(old_value - new_value) > error)
                    repeat = true;
            }
        }
    }


    vector<string> get_parameters(){
        return index_params;
    }
    
    float get_percent_not_taken(){
        float total = 0.0;
        float not_taken = 0.0;
        for (auto& s:states){
            for (auto& qs:s.get_qstates()){
                total = total + 1.0;
                if (qs.get_num_taken() == 0)
                    not_taken = not_taken + 1.0;
            }
        }
        return not_taken / total;
    }

    void print_model(bool detailed=false){
        for (auto& s:states){
            if (detailed){
                s.print_detailed();
                cout << endl;
            }
            else
                cout << s << endl;
        }
    }

    void update_bounds(){
        float t = 0.0;
        float curr_max = -INFINITY;
        float curr_min = INFINITY;
        bool f = false;
        for (auto& s:states){
            s.max_lower_bound = -INFINITY;
            for (auto& qs:s.qstates){//for every QState calculate upper and lower bounds of Q(s,a)
                for (int i=0; i < states.size(); i++){
                    t = qs.get_transition(i);
                    if (t != 0.0){//calculate max/min of V(s') accessible from current state s
                        curr_max = max(curr_max, states[i].get_value());
                        curr_min = min(curr_min, states[i].get_value());
                        f = true;
                    }
                }
                if (f){//if the state is not terminal, calculate max/min of rewards and add to the values calculated above to get bounds
                qs.upper_bound = *max_element(qs.rewards.begin(), qs.rewards.end()) + discount * curr_max;
                qs.lower_bound = *min_element(qs.rewards.begin(), qs.rewards.end()) + discount * curr_min;
                }
                else{//if the state is terminal, set upper bound to highest posssible and lower bound to lowest possible
                    qs.upper_bound = INFINITY;
                    qs.lower_bound = -INFINITY;
                }

                curr_max = -INFINITY;
                curr_min = INFINITY;
                f=false;
                if (qs.lower_bound > s.max_lower_bound) s.max_lower_bound = qs.lower_bound;//calculate maximum of upper bounds of a state, useful for value iteration condition
        }
    }
    }
        
};
