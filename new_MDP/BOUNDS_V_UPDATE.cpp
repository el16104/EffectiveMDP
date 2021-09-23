    void _v_update(State &state){
        //check upper and lower bounds here
        //for (int i=0; i< state.get_qstates().size(); i++)
        for (auto& qs:state.get_qstates())
        {
            //cout << "QSTATE UPPER BOUND: " << qs.upper_bound << "\t QSTATE LOWER BOUND: " << qs.lower_bound << "\t QVALUE: " << qs.qvalue << endl; 
            //if (state.get_qstates()[i].upper_bound > state.max_lower_bound)_q_update(state.get_qstates()[i]);
            //if (qs.upper_bound > state.max_lower_bound)_q_update(qs);
            //else state.get_qstates().erase(state.get_qstates().begin() + i);
            //else cout << "QState skipped" << endl;
            _q_update(qs);
        }
        state.update_value();
    }