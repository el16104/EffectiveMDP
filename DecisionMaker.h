#include <iostream>
#include "MDPModel.h"
#include "ModelConf.h"
#include <string>

using namespace std;


class DecisionMaker{
    public:
            
        string training_file;
        int last_meas;
        string model_type;
        json model_conf;
        bool do_vi;
        MDPModel model;
            
        DecisionMaker(string conf_file, string trainingfile){
            training_file = trainingfile;
            ModelConf conf(conf_file);
            model_type = conf.get_model_type();
            model_conf = conf.get_model_conf();
            if (model_type == "mdp"){
                do_vi = true;
                model = MDPModel(model_conf);
            }

        }

        void train(){
            cout << "Starting training..." << endl;
            int num_exp = 0;
            int skipped_exp = 0;
            ifstream file(training_file);
            int counter = 0;

            json old_meas;
            json new_meas;
            string typ;
            int val;
            pair<string,int> aux;
            double reward;
            vector<pair<string, int>> available_actions;

            if (file.is_open()) {
                string line;
                while (getline(file, line)) {
                    switch(counter % 4){
                        case 0:
                            old_meas = json::parse(line);
                            break;
                        case 1:
                            typ = line;
                            break;
                        case 2:
                            val = stoi(line);
                            aux = std::make_pair(typ, val);
                            break;
                        case 3:
                            new_meas = json::parse(line);
                            add_network_usage( old_meas );
                            add_network_usage( new_meas );
                            reward = get_reward(new_meas, aux);
                            model.set_state(old_meas);
                            available_actions = model.get_legal_actions();
                            for (auto& action:available_actions){
                                if (action.first != aux.first || action.second != aux.second){
                                    skipped_exp++;
                                    continue;
                                }
                                model.update(aux, new_meas, reward);
                                num_exp++;
                                if (num_exp % 100 == 0 && do_vi){
                                    model.value_iteration(0.1);
                                }


                            }
                            break;
                    }
                    counter++;
                    
                }
            }
        }

        void set_state(json measurements){
            add_network_usage(measurements);
            last_meas = measurements;
            model.set_state(measurements);

        }

        void add_network_usage(json measurements){
            measurements["network_usage"] = (double)measurements["bytes_in"] + (double)measurements["bytes_out"];
        }

        void update(pair<string,int> action, json meas, double reward = -1.0){
            add_network_usage(meas);
            if (reward < 0)
                reward = get_reward(meas, action);
            last_meas = meas;
            model.update(action, meas, reward);
        }

        MDPModel get_model(){
            return model;
        }

        double get_reward(json measurements, pair<string,int> action){
            double vms = (double)measurements["number_of_VMs"];
            double throughput = (double)measurements["total_throughput"];
            double reward = throughput - 800 * vms;
            return reward;
        }
        
        vector<pair<string,int>> get_legal_actions(){
            return model.get_legal_actions();
        }

        pair<string,int> suggest_action(){
            return model.suggest_action();
        }


        //set_value_iteration leipei epeidh leipei kai apo to montelo


};