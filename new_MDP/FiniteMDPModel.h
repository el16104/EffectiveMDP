#include <iostream>
#include <vector>
#include <map>
#include <stdexcept>
#include <string>
#include <stack>
#include <random>
#include <math.h>
#include <array>
#include <chrono>
#include <sstream>

#include "MDPModel.h"
#include "Complex.h"

#include "stdlib.h"
#include "stdio.h"


#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#include <nlohmann\json.hpp>
#endif

#ifdef linux
#include <nlohmann/json.hpp>
#define SIZE_T int
#endif

using namespace std::chrono;
enum model_type {infinite, naive, root, tree, inplace,infiniteM};

SIZE_T getValue(){
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc)))
        return(pmc.WorkingSetSize); //Value in Bytes!
    else
        return 0;
#endif
#ifdef linux
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
#endif

}

using json = nlohmann::json;

using namespace std;

class FiniteMDPModel: public MDPModel{
    public:
        stack<int> index_stack;
        stack<vector<pair<int , float>>> finite_stack;
        stack<vector<int>> action_stack; //STACK TO CONTAIN VECTOR OF BEST QSTATE FOR EACH INDEX
        float total_reward = 0.0;
        int max_memory_used = 0;
        int init_memory_used=0;
        int max_stack_memory = 0;
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

    /*
    New function, checks if the current memory used by the process is greater than
    the current maximum and updates it accordingly, using the getValue() function.
    No input.
    No output.
    */
    void checkMemoryUsage(){
        SIZE_T x = getValue();
        if (getValue() > max_memory_used){
            max_memory_used = getValue();
        }
    }

    /*
    New function, checks if the current number of elements in the stack is greater than
    the current maximum and updates it accordingly.
    No input.
    No output.
    */
    void checkStackSize(){
        if (stack_memory > max_stack_memory){
            max_stack_memory = stack_memory;
        }
    }


    void _q_update_finite(QState &qstate, vector<pair<int,float>> &V){
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

    void _q_update_finite(QState &qstate, vector<pair<int,float>> &V, int time_step){
        float new_qvalue = 0.0;
        float r;
        float t;
        for (int i=0; i < V.size(); i++){
            t = qstate.get_transition(i);
            r = qstate.get_reward(i, time_step);
            new_qvalue += t * (r +V[i].second);
        }
        qstate.set_qvalue(new_qvalue);
    }


    vector<pair<int, float>> calculateValues(int k, int starting_index, vector<pair<int, float>> V, bool tree = false){
        vector<pair<int,float>> V_tmp;
        //V_tmp.reserve(states.size());      
        V_tmp = V;
        for (int i = starting_index+1 ; i < k+1; i++){
            for (int j = 0 ; j < states.size(); j++ ){
                for (int m = 0; m < states[j].get_qstates().size(); m++){
                    _q_update_finite(states[j].qstates[m], V_tmp, i);
                }
                states[j].update_value();
            }
            V_tmp = getStateValues(states);
            if (!tree){
                index_stack.push(i);
                finite_stack.push(V_tmp);
                stack_memory++;
                checkStackSize();
                }
            }

        checkMemoryUsage();

        return V_tmp;

    }
    void calculateValuestest(int k, int starting_index, vector<pair<int, float>> &V, bool tree = false){

        for (int i = starting_index+1 ; i < k+1; i++){
            for (int j = 0 ; j < states.size(); j++ ){
                
                for (int m = 0; m < states[j].get_qstates().size(); m++){
                    _q_update_finite(states[j].qstates[m], V, i);
                }
                states[j].update_value();
            }
            V = getStateValuestest(states);
            if (!tree){
                index_stack.push(i);
                finite_stack.push(V);
                //stack_memory++;
               // checkStackSize();
                }
            }

        checkMemoryUsage();

    }

        void calculateValuestestcorrR(int k, int starting_index, vector<pair<int, float>> &V, bool tree = false){
            float num0rew=-1;
            int statenum=-1;
            for (int i = starting_index+1 ; i < k+1; i++){
                num0rew=calcrewa(V);
            for (int j = 0 ; j < states.size(); j++ ){
                
            for (int n = 0; n < states[j].get_qstates().size(); n++){
                    float new_qvalue = 0.0;
                    float r;
                    float t;
                    if (states[j].num_visited==0)
                        states[j].qstates[n].set_qvalue(num0rew);
                    else{
                    for (int m=0; m < states[j].qstates[n].trans.size(); m++){ //FOR EVERY ACCESIBLE STATE FROM CURRENT QSTATE
                        t = states[j].qstates[n].trans[m];
                        statenum=states[j].qstates[n].transtate[m];
                        r = states[j].qstates[n].get_reward(statenum, i);
                        new_qvalue += t * (r +V[statenum].second);
                    }
                    states[j].qstates[n].set_qvalue(new_qvalue);
                }
                }
                states[j].update_value();
            }
            V = getStateValuestest(states);
            if (!tree){
                //index_stack.push(i);
                finite_stack.push(V);
                //stack_memory++;
               // checkStackSize();
                }
            }

        checkMemoryUsage();

    }
    void calculateValuestestcorr(int k, int starting_index, vector<pair<int, float>> &V, bool tree = false){
            float num0rew=-1;
            int statenum=-1;
            for (int i = starting_index+1 ; i < k+1; i++){
                num0rew=calcrewa(V);
            for (int j = 0 ; j < states.size(); j++ ){
                
            for (int n = 0; n < states[j].get_qstates().size(); n++){
                    float new_qvalue = 0.0;
                    float r;
                    float t;
                    if (states[j].num_visited==0)
                        states[j].qstates[n].set_qvalue(num0rew);
                    else{
                    for (int m=0; m < states[j].qstates[n].trans.size(); m++){ //FOR EVERY ACCESIBLE STATE FROM CURRENT QSTATE
                        t = states[j].qstates[n].trans[m];
                        statenum=states[j].qstates[n].transtate[m];
                        r = states[j].qstates[n].get_reward(statenum, i);
                        new_qvalue += t * (r +V[statenum].second);
                    }
                    states[j].qstates[n].set_qvalue(new_qvalue);
                }
                }
                states[j].update_value();
            }
            V = getStateValuestest(states);
            if (!tree){
                index_stack.push(i);
                finite_stack.push(V);
                //stack_memory++;
               // checkStackSize();
                }
            }

        checkMemoryUsage();

    }

       pair<std::string,int> finite_suggest_action(){
        return states[current_state_num].get_optimal_action();
    }

    void takeAction(bool isInfinite, int time_step, bool p=false){
        pair<std::string,int> action;
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
                            reward = states[prev_state_num].qstates[i].get_reward(current_state_num, time_step);
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

    void takeAction2(int corraction, int time_step){
        float reward;
        int prev_state_num = current_state_num;
        float x = unif(eng);
        float acc = 0.0;
            for (int j=0; j<states[prev_state_num].qstates[corraction].trans.size();j++){
                    acc += states[prev_state_num].qstates[corraction].trans[j];
                    if (x < acc){
                        current_state_num = states[prev_state_num].qstates[corraction].transtate[j];
                        reward = states[prev_state_num].qstates[corraction].get_reward(current_state_num, time_step);
                        break;
                    }
                }
        total_reward += reward;
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

            stack_memory += V.size() * sizeof(pair<int,float>);

            if (V[initial_state_num].second > expected_reward) expected_reward = V[initial_state_num].second;

            traverseTree(k+1, r);

            //states = V;
            loadValueFunction(V);
            takeAction(false, k);

            finite_stack.pop();//remove top of the stack from memory
            index_stack.pop();

            stack_memory -= V.size() * sizeof(pair<int,float>);

            traverseTree(l, k-1);
        }
    }

    void naiveEvaluation(int horizon){
            //vector<State> V;

            vector<pair<int,float>> V;
            //V.reserve(states.size());
            resetValueFunction();
            //V = calculateValues(horizon, 0, states);
            V = calculateValues(horizon, 0, getStateValues(states));
            expected_reward = V[initial_state_num].second;
            while (!finite_stack.empty()){

                steps_made++;

                //states = finite_stack.top();
                loadValueFunction(finite_stack.top());
                //takeAction(false);
                finite_stack.pop();
                index_stack.pop();
                stack_memory--;
            }
    }

    /*
    Auxiliary function that returns the Value Function for every state of the model.
    No input.
    Returns a vector containing the Value Function value for every state of the model.
    */
    vector<float> getStateValueFunction(){
        vector<float> values;
        //values.reserve(states.size());    

        for (int i=0; i < states.size(); i++){
            values.push_back(states[i].get_value());
        }
        
        return values;
    }

    /*
    Auxiliary function that returns the best QState for every state of the model.
    No input.
    Returns a vector containing the index of the best QState for every state of the model.
    */
    vector<int> getStateActions(){
        vector<int> values;
        //values.reserve(states.size());    

        for (int i=0; i < states.size(); i++){
            values.push_back(states[i].get_best_qstate());
        }
        
        return values;
    }

    /*
    New function, does the same as calculateValues(), but stores in memory (stack) only the bestQState.
    Takes as argument the horizon of the Finite-Horizon MDP.
    Returns the total reward the agent is EXPECTED to collect.
    */
    float calculatePolicy(int k){
        vector<float> V_tmp;
        //V_tmp.reserve(states.size());      
        V_tmp = getStateValueFunction();
        for (int i = 1 ; i < k+1; i++){ //FOR EVERY INDEX UP TO THE HORIZON
            for (int j = 0 ; j < states.size(); j++ ){ //FOR EVERY STATE
                for (int n = 0; n < states[j].get_qstates().size(); n++){ //FOR EVERY QSTATE OF EACH STATE
                    float new_qvalue = 0.0;
                    float r;
                    float t;
                    for (int m=0; m < V_tmp.size(); m++){ //FOR EVERY ACCESIBLE STATE FROM CURRENT QSTATE
                        t = states[j].qstates[n].get_transition(m);
                        r = states[j].qstates[n].get_reward(m, i);
                        new_qvalue += t * (r +V_tmp[m]);
                    }

                    states[j].qstates[n].set_qvalue(new_qvalue);
                }
                states[j].update_value();
            }
            V_tmp = getStateValueFunction();

            action_stack.push(getStateActions());
            stack_memory++;
            checkStackSize();
            checkMemoryUsage();
        }
        return V_tmp[initial_state_num];
    }
    float calcrewa(vector<float> &V_tmp){
        float new_qvalue=0;
        for (int m=0; m < V_tmp.size(); m++){ //FOR EVERY ACCESIBLE STATE FROM CURRENT QSTATE
                        new_qvalue += (V_tmp[m]);
                    }
        return new_qvalue/states.size();
    }
    float calcrewa(vector<pair<int,float>> V_tmp){
        float new_qvalue=0;
        for (int m=0; m < V_tmp.size(); m++){ //FOR EVERY ACCESIBLE STATE FROM CURRENT QSTATE
                        new_qvalue += (V_tmp[m].second);
                    }
        return new_qvalue/states.size();
    }
    float calculatePolicycorr(int k){
        vector<float> V_tmp;
        //V_tmp.reserve(states.size());      
        V_tmp = getStateValueFunction();
        int statenum;
        float num0rew=0;
        for (int i = 1 ; i < k+1; i++){ //FOR EVERY INDEX UP TO THE HORIZON
                 num0rew=calcrewa(V_tmp);  
            for (int j = 0 ; j < states.size(); j++ ){ //FOR EVERY STATE   
                     
                for (int n = 0; n < states[j].get_qstates().size(); n++){ //FOR EVERY QSTATE OF EACH STATE
                    float new_qvalue = 0.0;
                    float r;
                    float t;
                    if (states[j].num_visited==0)
                        states[j].qstates[n].set_qvalue(num0rew);
                    else{
                    for (int m=0; m < states[j].qstates[n].trans.size(); m++){ //FOR EVERY ACCESIBLE STATE FROM CURRENT QSTATE
                        t = states[j].qstates[n].trans[m];
                        statenum=states[j].qstates[n].transtate[m];
                        r = states[j].qstates[n].get_reward(statenum, i);
                        new_qvalue += t * (r +V_tmp[statenum]);
                    }
                    states[j].qstates[n].set_qvalue(new_qvalue);
                }
                }
                states[j].update_value();
            }
            V_tmp = getStateValueFunction();

            action_stack.push(getStateActions());
            stack_memory++;
            checkStackSize();
            checkMemoryUsage();
        }
        return V_tmp[initial_state_num];
    }
    /*
    Runs the Naive Finite-Horizon MDP method using calculatePolicy.
    Takes as argument the Finite-Horizon MDP's horizon;
    */
    void naiveEvaluation2(int horizon){
        resetValueFunction();
        auto start22 = std::chrono::high_resolution_clock::now();
        expected_reward = calculatePolicy(horizon);
        auto end22 = std::chrono::high_resolution_clock::now();     // the new current "timepoint"
        auto elapsed = end22 - start22;                 // difference is a "duration"
        std::cout << "hoho" << ": " << elapsed.count()* 0.000001 << '\n';  // clock ticks (seconds)
    
        while (!action_stack.empty()){
            steps_made++;
            loadBestQStates(action_stack.top());
            //takeAction(false);
            action_stack.pop();
            stack_memory--;
        }
    }
    void naiveEvaluationcorr(int horizon){
        resetValueFunction();
        auto start22 = std::chrono::high_resolution_clock::now();
        expected_reward = calculatePolicycorr(horizon);
        auto end22 = std::chrono::high_resolution_clock::now();     // the new current "timepoint"
        auto elapsed = end22 - start22;                 // difference is a "duration"
        std::cout << "hoho" << ": " << elapsed.count()* 0.000001 << '\n';  // clock ticks (seconds)
    
        int actiont=-1;
        vector<int> V;
        steps_made = 0;
        while (!action_stack.empty()){
            V=action_stack.top();
            actiont=V[current_state_num];
            takeAction2(actiont, horizon - steps_made);
            action_stack.pop();
            steps_made++;
            stack_memory--;
        }
    }



    void inPlaceEvaluation(int horizon){

        vector<pair<int,float>> V;
        vector<pair<int,float>> Ve;
        int steps_remaining = horizon;
        resetValueFunction();
        vector<pair<int,float>> values;
        //values.reserve(states.size());
        V = calculateValues(steps_remaining, 0, getStateValues(states),true);
        expected_reward = V[initial_state_num].second;
        loadValueFunction(V);
        //takeAction(false);
        steps_made++;
        steps_remaining--;
        checkMemoryUsage();
        int a=getValue();
        while(steps_remaining > 0){
            V = calculateValues(steps_remaining, 0, getStateValues(states),true);
            a=getValue();
            loadValueFunction(V);
            //takeAction(false);
            steps_made++;
            steps_remaining--;
        }
    }
       void inPlaceEvaluation3(int horizon){
        vector<pair<int,float>> V;
        //V_temp.reserve(states.size()); 
        //V.reserve(states.size()); 
        int steps_remaining = horizon;
        resetValueFunction();
        V =getStateValuestest(states);
        calculateValuestestcorrR(steps_remaining, 0, V,true);
        expected_reward = V[initial_state_num].second;
        loadValueFunctiontest(V);
        //takeAction(false, horizon);
        takeAction2(V[current_state_num].first, steps_remaining);
        steps_made++;
        steps_remaining--;
        if (getValue() > max_memory_used){
            max_memory_used = getValue();
        }
        while(steps_remaining > 0){
            resetValueFunction();
            V=getStateValuestest(states);
            calculateValuestestcorrR(steps_remaining, 0, V,true);
            loadValueFunctiontest(V);
            //takeAction(false, steps_remaining);
            takeAction2(V[current_state_num].first, steps_remaining);
            steps_made++;
            steps_remaining--;
        }
    }

    void inPlaceEvaluation2(int horizon){

        vector<pair<int,float>> V;
        vector<pair<int,float>> Ve;
        int steps_remaining = horizon;
        resetValueFunction();
        vector<pair<int,float>> values;
        //values.reserve(states.size());
        vector<pair<int,float>> V_tmp;
        V=getStateValues1(states,values);
        //calculateValues now
        V;
        int a;
        for (int i = 1 ; i < steps_remaining+1; i++){
            for (int j = 0 ; j < states.size(); j++ ){
                for (int m = 0; m < states[j].get_qstates().size(); m++){
                    _q_update_finite(states[j].qstates[m], V, i);
                }
                states[j].update_value();
            }
            a=getValue();
            //V_tmp.clear();
            for (int i=0; i < V.size(); i++)
                values.push_back(make_pair( states[i].get_best_qstate(), states[i].get_value()));
            V=values;
            values.clear();
            a=getValue();
            }

        checkMemoryUsage();
        //calculateValues finished
        expected_reward = V[initial_state_num].second;
        loadValueFunction(V);
        //takeAction(false);
        steps_made++;
        steps_remaining--;
        checkMemoryUsage();
        a=getValue();
        while(steps_remaining > 0){

            for (int i = 1 ; i < steps_remaining+1; i++){
            for (int j = 0 ; j < states.size(); j++ ){
                for (int m = 0; m < states[j].get_qstates().size(); m++){
                    _q_update_finite(states[j].qstates[m], V, i);
                }
                states[j].update_value();
            }
            a=getValue();
            //V_tmp.clear();
            //V = getStateValues(states);
            //getStateValues
            for (int i=0; i < V.size(); i++)
                values.push_back(make_pair( states[i].get_best_qstate(), states[i].get_value()));
            V=values;
            values.clear();
        }
            a=getValue();
            
            loadValueFunction(V);
            //takeAction(false);
            steps_made++;
            steps_remaining--;
        }
    }



void rootEvaluation2(int horizon){

        vector<pair<int,float>> V;
        //V.reserve(states.size());
        //V.reserve(states.size());
        int steps_remaining = horizon;
        //resetValueFunction();
        int floor_of_square_root = floor(sqrt(horizon));
        int i=0;
        V=getStateValuestest(states);
        for ( ;i+floor_of_square_root <= horizon; i=i+floor_of_square_root){
            calculateValuestest(i+floor_of_square_root,i,  V, true);
            finite_stack.push(V);
            //stack_memory++;
        }
        if (i!=horizon){
            V=getStateValuestest(states);
            calculateValuestest(horizon, i, V);
        }
        expected_reward = V[initial_state_num].second;
        //checkStackSize();
        checkMemoryUsage();
        for ( ;i < horizon; i=i+1){ 
           
            loadValueFunction(V);
            //takeAction(false);
            finite_stack.pop();
            //stack_memory--;
            //steps_made++;
            steps_remaining--;
            V = finite_stack.top();  
        }
        while(steps_remaining > 0){
            if (finite_stack.empty()){
                resetValueFunction();
                V=getStateValuestest(states);
                calculateValuestest(steps_remaining, 0, V);
            }
            else{

                if( (steps_remaining+1)%floor_of_square_root==0){
                    V=finite_stack.top();
                    calculateValuestest(steps_remaining, steps_remaining-floor_of_square_root+1,V );
                    //checkMemoryUsage();
                }
                else
                    V = finite_stack.top();            
            }
            loadValueFunction(V);
            //takeAction(false);
            //checkStackSize();
            checkMemoryUsage();
            finite_stack.pop();
            //stack_memory--;
            //steps_made++;
            steps_remaining--;
        }
    }

void rootEvaluationcorr(int horizon){

        vector<pair<int,float>> V;
        //V.reserve(states.size());
        //V.reserve(states.size());
        int steps_remaining = horizon;
        int actiont=-1;
        resetValueFunction();
        int floor_of_square_root = floor(sqrt(horizon));
        int i=0;
        V=getStateValuestest(states);
        for ( ;i+floor_of_square_root <= horizon; i=i+floor_of_square_root){
            calculateValuestestcorrR(i+floor_of_square_root,i,  V, true);
            finite_stack.push(V);
            //stack_memory++;
        }
        if (i!=horizon){
            V=getStateValuestest(states);
            calculateValuestestcorrR(horizon, i, V);
        }
        expected_reward = V[initial_state_num].second;
        //checkStackSize();
        checkMemoryUsage();
        for ( ;i < horizon; i=i+1){ 
           
            loadValueFunction(V);
            takeAction2(V[current_state_num].first, steps_remaining);
            //takeAction(false, horizon-i);
            finite_stack.pop();
            //stack_memory--;
            //steps_made++;
            steps_remaining--;
            V = finite_stack.top();  
        }
        while(steps_remaining > 0){
            if (finite_stack.empty()){
                resetValueFunction();
                V=getStateValuestest(states);
                calculateValuestestcorrR(steps_remaining, 0, V);
            }
            else{

                if( (steps_remaining+1)%floor_of_square_root==0){
                    V=finite_stack.top();
                    calculateValuestestcorrR(steps_remaining, steps_remaining-floor_of_square_root+1,V );
                    //checkMemoryUsage();
                }
                else
                    V = finite_stack.top();            
            }
            loadValueFunctiontest(V);
            takeAction2(V[current_state_num].first, steps_remaining);
            //checkStackSize();
            checkMemoryUsage();
            finite_stack.pop();
            //stack_memory--;
            //steps_made++;
            steps_remaining--;
        }
    }

    void rootEvaluation(int horizon){

        vector<pair<int,float>> V;
        //V.reserve(states.size());
        int steps_remaining = horizon;
        resetValueFunction();

        V = calculateValues(1, 0, getStateValues(states),true);
        int floor_of_square_root = floor(sqrt(horizon));

        for (int i = 2; i <= horizon; i++){
            V = calculateValues(i, i-1, V, true);
            if (i % floor_of_square_root == 0){
                finite_stack.push(V);
                index_stack.push(i);
                stack_memory++;
                checkStackSize();
            }
        }
        expected_reward = V[initial_state_num].second;

        while(steps_remaining > 0){
            if (finite_stack.empty()){
                resetValueFunction();
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
            //takeAction(false);

            checkMemoryUsage();

            finite_stack.pop();
            index_stack.pop();

            stack_memory--;

            steps_made++;
            steps_remaining--;
        }
    }



    void printValueFunction(){
        if(!finite_stack.empty()){
            for (int i = 0; i < finite_stack.top().size(); i++){
                cout << "State " << i << ": " << finite_stack.top()[i].second << endl;
            }
        }
        else{
            for (int i = 0; i < states.size(); i++){
                cout << "State " << states[i].get_state_num()<< ": " << states[i].value << endl;
            }  
        }
    }

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


    //calculates Value Function for target index while saving every intermediate index needed
    void treeTraversal1(int target, int horizon, vector<pair<int,float>> &V){
        int l = 0;
        int r = horizon;

        //V.reserve(states.size());
       // V_tmp.reserve(states.size());
        int k = (l + r)/2;
        if (!index_stack.empty()){
            if (index_stack.top() == target){
                V = finite_stack.top();
                finite_stack.pop();
                index_stack.pop();
                //stack_memory--;
                return ;
            }
            else{
                k = index_stack.top();
            }
        }


        while ( l <= r){
            if (k == target){
                if (finite_stack.empty()){
                    resetValueFunction();//if no vector is saved in memory, calculate objective from the beginning
                    V=getStateValuestest(states);
                    calculateValuestestcorrR(k, 0, V, true);

                }
                else{
                    V=finite_stack.top();
                    calculateValuestestcorrR(k, index_stack.top(),V , true);//use last saved vector in memory to calculate objective
                    
                }
                break;
            }
            else if ( k < target){
                if (finite_stack.empty()){
                    resetValueFunction();//if no vector is saved in memory, calculate objective from the beginning
                    V=getStateValuestest(states);
                    calculateValuestestcorrR(k, 0, V, true);
                    finite_stack.push(V);
                    index_stack.push(k);
                    //stack_memory++;
                    //checkStackSize();
                }
                else{
                    if (index_stack.top() != k){
                        V=finite_stack.top();
                        calculateValuestestcorrR(k, index_stack.top(), V, true);
                        finite_stack.push(V);//use last saved vector in memory to calculate objective
                        index_stack.push(k);
                        //stack_memory++;
                        //checkStackSize();
                    }
                }
                l = k + 1;
                k = (l + r)/2;
            }
            else if ( k > target){
                r = k - 1;
                k = (l + r)/2;
            }
        }
        checkMemoryUsage();
        
    }
    void treeTraversalcorr(int target, int horizon, vector<pair<int,float>> &V){
        int l = 0;
        int r = horizon;

        //V.reserve(states.size());
       // V_tmp.reserve(states.size());
        int k = (l + r)/2;
        if (!index_stack.empty()){
            if (index_stack.top() == target){
                V = finite_stack.top();
                finite_stack.pop();
                index_stack.pop();
                //stack_memory--;
                return ;
            }
            else{
                k = index_stack.top();
            }
        }


        while ( l <= r){
            if (k == target){
                if (finite_stack.empty()){
                    resetValueFunction();//if no vector is saved in memory, calculate objective from the beginning
                    V=getStateValuestest(states);
                    calculateValuestestcorr(k, 0, V, true);

                }
                else{
                    V=finite_stack.top();
                    calculateValuestestcorr(k, index_stack.top(),V , true);//use last saved vector in memory to calculate objective
                    
                }
                break;
            }
            else if ( k < target){
                if (finite_stack.empty()){
                    resetValueFunction();//if no vector is saved in memory, calculate objective from the beginning
                    V=getStateValuestest(states);
                    calculateValuestestcorr(k, 0, V, true);
                    finite_stack.push(V);
                    index_stack.push(k);
                    //stack_memory++;
                    //checkStackSize();
                }
                else{
                    if (index_stack.top() != k){
                        V=finite_stack.top();
                        calculateValuestestcorr(k, index_stack.top(), V, true);
                        finite_stack.push(V);//use last saved vector in memory to calculate objective
                        index_stack.push(k);
                        //stack_memory++;
                        //checkStackSize();
                    }
                }
                l = k + 1;
                k = (l + r)/2;
            }
            else if ( k > target){
                r = k - 1;
                k = (l + r)/2;
            }
        }
        checkMemoryUsage();
        
    }

    void treeEvaluation(int horizon){
        int steps_remaining = horizon;
        vector<pair<int,float>> V;
        //V.reserve(states.size());
        while(steps_remaining > 0){
            V = treeTraversal(steps_remaining, horizon);
            if (steps_remaining == horizon)
                expected_reward = V[initial_state_num].second;
            loadValueFunction(V);
            //takeAction(false);
            steps_remaining--;
        }
    }
        void treeEvaluation2(int horizon){
        int steps_remaining = horizon;
        vector<pair<int,float>> V;
        //V.reserve(states.size());
        while(steps_remaining > 0){
            treeTraversal1(steps_remaining, horizon,V);
            if (steps_remaining == horizon)
                expected_reward = V[initial_state_num].second;
            loadValueFunctiontest(V);
            takeAction(false, steps_remaining);
            checkMemoryUsage();
            steps_remaining--;
        }
    }
 void treeEvaluationcorr(int horizon){
        int steps_remaining = horizon;
        int actiont=-1;
        vector<pair<int,float>> V;
        //V.reserve(states.size());
        while(steps_remaining > 0){
            treeTraversalcorr(steps_remaining, horizon,V);
            if (steps_remaining == horizon)
                expected_reward = V[initial_state_num].second;
            loadValueFunctiontest(V);
            takeAction2(V[current_state_num].first, steps_remaining);
            checkMemoryUsage();
            steps_remaining--;
        }
    }

    vector<pair<int,float>> treeTraversal(int target, int horizon){
        int l = 0;
        int r = horizon;
        vector<pair<int,float>> V;
        //V.reserve(states.size());
        int k = (l + r)/2;
        if (!index_stack.empty()){
            if (index_stack.top() == target){
                V = finite_stack.top();
                finite_stack.pop();
                index_stack.pop();
                stack_memory -= states.size()*sizeof(pair<int,float>);
                return V;
            }
            else{
                k = index_stack.top();
            }
        }
        while ( l <= r){
            if (k == target){
                if (finite_stack.empty()){
                    resetValueFunction();//if no vector is saved in memory, calculate objective from the beginning
                    V = calculateValues(k, 0, getStateValues(states), true);
                }
                else{
                    V = calculateValues(k, index_stack.top(), finite_stack.top(), true);//use last saved vector in memory to calculate objective
                    
                }
                break;
            }
            else if ( k < target){
                if (finite_stack.empty()){
                    resetValueFunction();//if no vector is saved in memory, calculate objective from the beginning
                    finite_stack.push(calculateValues(k, 0, getStateValues(states), true));
                    index_stack.push(k);
                    stack_memory += states.size()*sizeof(pair<int,float>);
                if (stack_memory > max_stack_memory)
                    max_stack_memory = stack_memory;
                }
                else{
                    if (index_stack.top() != k){
                        finite_stack.push(calculateValues(k, index_stack.top(), finite_stack.top(), true));//use last saved vector in memory to calculate objective
                        index_stack.push(k);
                        stack_memory += states.size()*sizeof(pair<int,float>);
                        if (stack_memory > max_stack_memory)
                            max_stack_memory = stack_memory;
                    }
                }
                l = k + 1;
                k = (l + r)/2;
            }
            else if ( k > target){
                r = k - 1;
                k = (l + r)/2;
            }
        }
        
        return V;
    }


    void infiniteEvaluation(int horizon){
        resetValueFunction();
        max_memory_used=value_iteration(0.1, false);
        max_memory_used = getValue();
        expected_reward = states[initial_state_num].value;
        for (int time = horizon; time > 0; time--){
            takeAction(true, time);
            }
    }
    
    void infiniteMEvaluation(int horizon){
        vector<pair<int,float>> V;
        int steps_remaining = horizon;
        resetValueFunction();
        V =getStateValuestest(states);
        calculateValuestestcorrR(steps_remaining, 0, V,true);
        expected_reward = V[initial_state_num].second;
        loadValueFunctiontest(V);
        takeAction2(V[current_state_num].first, steps_remaining);
        steps_made++;
        steps_remaining--;
        if (getValue() > max_memory_used){
            max_memory_used = getValue();
        }
        while(steps_remaining > 0){
            //resetValueFunction();
            //V=getStateValuestest(states);
            //calculateValuestestcorrR(steps_remaining, 0, V,true);
            //loadValueFunctiontest(V);
            //takeAction(false, steps_remaining);
            takeAction2(V[current_state_num].first, steps_remaining);
            steps_made++;
            steps_remaining--;
        }
    }

    void infiniteEvaluationM(int horizon){
        resetValueFunction();
        value_iterationM(horizon);
        max_memory_used = getValue();
        expected_reward = states[initial_state_num].value;
        for (int time = horizon; time > 0; time--){
            takeAction(true, time);
        }
    }

    void runAlgorithm(model_type alg, int horizon=100){

        auto start = high_resolution_clock::now();
        switch(alg) {   
            case infinite:
                cout << "INFINITE MDP MODEL: " << endl;
                init_memory_used = getValue();
                infiniteEvaluation(horizon);

                break;
            case infiniteM:
                cout << "INFINITEM MDP MODEL: " << endl;
                init_memory_used = getValue();
                //infiniteEvaluationM(horizon);
                infiniteMEvaluation(horizon);

                break;
            case naive:
                cout << "NAIVE FINITE MDP MODEL: " << endl;
                init_memory_used = getValue();
                naiveEvaluationcorr(horizon);
                //naiveEvaluation2(horizon);

                break;
            case root:
                cout << "ROOT FINITE MDP MODEL: " << endl;
                init_memory_used = getValue();
                rootEvaluationcorr(horizon);
                //rootEvaluation2(horizon);

                break;
            case tree:
                cout << "TREE FINITE MDP MODEL: " << endl;
                init_memory_used = getValue();
                treeEvaluation2(horizon);
                //treeEvaluationcorr(horizon);

                break;
            case inplace:
                cout << "IN-PLACE FINITE MDP MODEL: " << endl;
                init_memory_used = getValue();
                inPlaceEvaluation3(horizon);

                break;
            default:
                cout << "Invalid Model Type. Valid model types are: infinite, naive, root, tree, inplace" << endl;
                return;
        }

        auto stop = high_resolution_clock::now();
        auto duration = duration_cast<microseconds>(stop - start);
        cout << "Horizon size: " << horizon << endl;
        cout << "Total Reward Expected: " << expected_reward << endl;
        cout << "Total Reward Collected: " << total_reward << endl;
        cout << "Execution time (sec): " << duration.count() * 0.000001 << endl;
        cout << "Peak memory used (MB): " << (max_memory_used) / 1000000.0 << endl;
        cout << "Peak memory used (MB): " << (init_memory_used) / 1000000.0 << endl;
         cout << "Peak memory used (MB): " << (max_memory_used-init_memory_used) << endl;
        //cout << "(" << horizon << "," << duration.count() * 0.000001 << ")";
        cout << endl;
    }



};
