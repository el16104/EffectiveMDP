#include <random>
#include <iostream>

using namespace std;

std::random_device rd;
std::default_random_engine eng(rd());
std::uniform_real_distribution<double> unif(0,1);

int main(){
    double x = unif(eng);
    while(x > 0.01){
        cout << "NOOOO " << x << endl;
        x = unif(eng);
    }
    cout << x << endl;
}