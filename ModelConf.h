#include <iostream>
#include <nlohmann/json.hpp>
#include <fstream>

// for convenience
using json = nlohmann::json;

using namespace std;


class ModelConf{
    public:
        json conf;
        string model;

        ModelConf(string conf_file){
            std::ifstream ifs(conf_file);
            json jf = json::parse(ifs);
            model = jf["model"];
            conf = jf;

        }

        json get_model_conf(){
            return conf;
        }

        string get_model_type(){
            return model;
        }
};