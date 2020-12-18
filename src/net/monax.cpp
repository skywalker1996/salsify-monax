#define CWND_MIN 30
#define CWND_MAX 1000
#define SAVEUTILITY 20
#define SAVEHISTORY 100
#define MID 15
#define HIGH 20
#define RTT_THRESHOLD 30
#define ENSEMBLE true
#define RTT_PREDICTION false
#define MONITOR_LEN 10

#include<iostream>
#include<string>
#include<algorithm>
#include<vector>
#include<map>
#include<functional>
#include<numeric>
#include<ctime>
#include<fstream>
#include<iterator>
#include<cmath>

#include "monax.h"

void Monax::setRealRtt(double RealRtt) {
    this->RealRtt = RealRtt;

    if (current_monitor_para["network_delay"].size() < MONITOR_LEN) {
        current_monitor_para["network_delay"].push_back(RealRtt);
    }
    else {
        current_monitor_para["network_delay"].erase(current_monitor_para["network_delay"].begin());
        current_monitor_para["network_delay"].push_back(RealRtt);
    }
}



void Monax::setFlightSize(int flightSize){
    this->flightSize = flightSize;

    if (current_monitor_para["inflight_size"].size() < MONITOR_LEN) {
        current_monitor_para["inflight_size"].push_back(flightSize);
    }
    else {
        current_monitor_para["inflight_size"].erase(current_monitor_para["inflight_size"].begin());
        current_monitor_para["inflight_size"].push_back(flightSize);
    }
}


void Monax::setDeliveryRate(double m_iDeliveryRate) {
    this->m_iDeliveryRate = m_iDeliveryRate;

    if (current_monitor_para["delivery_rate"].size() < MONITOR_LEN) {
        current_monitor_para["delivery_rate"].push_back(m_iDeliveryRate);
    }
    else {
        current_monitor_para["delivery_rate"].erase(current_monitor_para["delivery_rate"].begin());
        current_monitor_para["delivery_rate"].push_back(m_iDeliveryRate);
    }
}


void Monax::setSndPeriod(int SndPeriod) {
    this->SndPeriod = SndPeriod;

    if (current_monitor_para["send_period"].size() < MONITOR_LEN) {
        current_monitor_para["send_period"].push_back(SndPeriod);
    }
    else {
        current_monitor_para["send_period"].erase(current_monitor_para["send_period"].begin());
        current_monitor_para["send_period"].push_back(SndPeriod);
    }

}

void Monax::setWndSize(int WndSize) {
    this->currentCWND = WndSize;

    if (current_monitor_para["cwnd"].size() < MONITOR_LEN) {
        current_monitor_para["cwnd"].push_back(WndSize);
    }
    else {
        current_monitor_para["cwnd"].erase(current_monitor_para["cwnd"].begin());
        current_monitor_para["cwnd"].push_back(WndSize);
    }
}

void Monax::setSrtLossSeqStatus(bool LossFlag) {
    this->LossFlag = LossFlag;
}

void Monax::calcCCPara() {

    if (current_monitor_para["network_delay"].size() < MONITOR_LEN) {
        return;
    }

    Actiongroup actgroup = action(current_monitor_para);
    this->currentCWND = actgroup.cwnd;
    this->currentUtility = actgroup.utility;
}

int Monax::getSndPeriod() {
    return this->SndPeriod;
}

int Monax::getWndSize() {
    return this->currentCWND;
}


std::vector<int> getNonZeroIndex(std::vector<double> sample) {

    std::vector<int> tempVector;
    for (size_t i = 0; i < sample.size(); i++) {
        if (sample[i] != 0.0) { tempVector.push_back(i); }
    }
    return tempVector;
}

double check_accuracy(std::vector<int> prediction, std::vector<int> label) {

    int acc_count = 0;
    for (size_t i = 0; i < prediction.size(); i++) {
        if (prediction[i] == label[i]) {
            acc_count++;
        }
    }
    return acc_count / prediction.size();
}


double logistic(double var) {

    var = std::max(std::min(var, 100.0), -100.0);
    return 1.0 / (1 + exp(-var));
}

Predictiongroup predict(std::vector<double>sample, std::vector<double>weights) {

    double product = 0;
    for (size_t i = 0; i < sample.size(); i++) {
        product += sample[i] * weights[i];
    }
    double raw_output = logistic(product + weights[weights.size() - 1]);
    if (raw_output > 0.5) {
        return { 1,raw_output };
    }
    else {
        return { 0,raw_output };
    }
}


double MSE(double prediction, int label) {

    return 0.5 * pow((prediction - label), 2);
}


template<typename T>
inline size_t argmax(T first, T last) {
    return std::distance(first, std::max_element(first, last));
}


Monax::Monax(int dim, double alpha, double beta, double lambda1, double lambda2, double initial_cwnd) {
    inputDim = dim;
    m_alpha = alpha;
    m_beta = beta;
    m_lambda1 = lambda1;
    m_lambda2 = lambda2;
    currentCWND = initial_cwnd;
    model_num = 8;
    current_model = 0;

    RealRtt = 0.0;
    flightSize = 0;
    m_iDeliveryRate = 0.0;
    SndPeriod = 0.0;
    LossFlag = 0;
    currentUtility = 0.0;
    max_network_delay = 0.0;
    delivery_rate = 0.0;

    RttHistory = {};

    //CWND step size params
    increase_step = 10;
    decrease_step = 10;
    initial_step_size = 10;
    increase_step_change_rate = 10;
    decrease_step_change_rate = 10;
    stepsize_max = 30;

    //aggregation module intervention level (0.0 - 1.0)
    intervention_prob = 0.0;
    accuracy = 1;
    used_model = 0;

    coldstart = true;

    save_history = SAVEHISTORY;

    std::vector<double> zs(inputDim + 1);
    std::vector<double> ns(inputDim + 1);
    std::vector<double> weights(inputDim + 1);

    for (int i = 0; i < model_num; i++) {
        std::map<std::string, std::vector<double>> buffer;
        buffer.insert(std::pair<std::string, std::vector<double>>("_zs", zs));
        buffer.insert(std::pair<std::string, std::vector<double>>("_ns", ns));
        buffer.insert(std::pair<std::string, std::vector<double>>("weights", weights));
        modelPool.insert(std::pair<int, std::map<std::string, std::vector<double>>>(i, buffer));
    }


    model_history_prediction.resize(model_num);

    if (ENSEMBLE == true) {
        used_model = 0;
    }

    std::vector<double> m_network_delay;
    std::vector<double> m_packet_loss;
    std::vector<double> m_send_period;
    std::vector<double> m_delivery_rate;
    std::vector<double> m_queue_length;
    std::vector<double> m_inflight_size;
    std::vector<double> m_cwnd;


    current_monitor_para.insert(std::make_pair("network_delay", m_network_delay));
    current_monitor_para.insert(std::make_pair("packet_loss", m_packet_loss));
    current_monitor_para.insert(std::make_pair("send_period", m_send_period));
    current_monitor_para.insert(std::make_pair("delivery_rate", m_delivery_rate));
    current_monitor_para.insert(std::make_pair("queue_length", m_queue_length));
    current_monitor_para.insert(std::make_pair("inflight_size", m_inflight_size));
    current_monitor_para.insert(std::make_pair("cwnd", m_cwnd));




}


Actiongroup Monax::action(std::map< std::string, std::vector<double>> monitor_para) {

    int cwnd;
    double utility;
    int prediction;
    double prediction_prob = 0.0;

    int decision;
    double decision_prob;

    current_monitor_para = monitor_para;

    int delaysize = current_monitor_para["network_delay"].size();
    delivery_rate = monitor_para["delivery_rate"][0];
    network_delay_gradient = monitor_para["network_delay"][delaysize - 1] - monitor_para["network_delay"][delaysize - 2];
    //current_loss = monitor_para["packet_loss"][monitor_para["packet_loss"].size() - 1];
    max_network_delay = std::max(monitor_para["network_delay"][delaysize - 1], monitor_para["network_delay"][delaysize - 2]);
    //throughput_error = monitor_para["throughput_error"][0];
    currentCWND = int(monitor_para["cwnd"][0]);
    //RTT prediction module, skip now 124-130

    std::vector<double>sample = { delivery_rate,network_delay_gradient,monitor_para["network_delay"][MONITOR_LEN - 1] };//the size of sample should be inputDim
    utility = utility_function(sample, RTT_THRESHOLD, RTT_PREDICTION);

    if (coldstart) {

        int label = 1;

        history_label.push_back(0);
        utility_record.push_back(0);
        utility_record.push_back(0);

        model_history_prediction[current_model].push_back(0);
        history_prediction.push_back(0);
        history_decision.push_back(0);

        coldstart = false;
    }
    else {

        int label;
        if (utility - *(utility_record.end()-1) > 0) {
            label = *(history_decision.end()-1);
        }
        else { label = 1 - *(history_decision.end()-1); }

        if (history_label.size() < SAVEHISTORY) {
            history_label.push_back(label);
        }
        else {
            history_label.erase(history_label.begin());
            history_label.push_back(label);
        }

        if (utility_record.size() < SAVEUTILITY) {
            utility_record.push_back(utility);
        }
        else {
            utility_record.erase(utility_record.begin());
            utility_record.push_back(utility);
        }
    }
    //online learning FTRL algorithm
    std::vector<double>& para_zs = modelPool[current_model]["_zs"];
    std::vector<double>& para_ns = modelPool[current_model]["_ns"];
    std::vector<double>& para_weights = modelPool[current_model]["weights"];

    if (abs(para_zs[inputDim]) > m_lambda1) {
        double fore = m_beta + sqrt(para_ns[inputDim]) / m_alpha + m_lambda2;
        para_weights[inputDim] = -1.0 / fore * (para_zs[inputDim] - ((para_zs[inputDim] > 0) - (para_zs[inputDim] < 0)) * m_lambda1);
    }
    else {
        para_weights[inputDim] = 0.0;
    }

    std::vector<int> nonzeroVector = getNonZeroIndex(sample);
    for (std::vector<int>::iterator it = nonzeroVector.begin(); it != nonzeroVector.end(); it++) {
        if (abs(para_zs[*it]) > m_lambda1) {
            double fore = m_beta + sqrt(para_ns[*it]) / m_alpha + m_lambda2;
            para_weights[*it] = -1.0 / fore * (para_zs[*it] - ((para_zs[*it] > 0) - (para_zs[*it] < 0)) * m_lambda1);
        }
        else {
            para_weights[*it] = 0.0;
        }
    }

    double base_grad;
    if (ENSEMBLE == true) {
        base_grad = *(model_history_prediction[current_model].end() - 1) - *(history_label.end() - 1);
    }
    else {
        base_grad = *(history_prediction.end() - 1) - *(history_label.end() - 1);
        accuracy = check_accuracy(history_prediction, history_label);
    }


    //update current model parameters
    double sigma;
    for (std::vector<int>::iterator it = nonzeroVector.begin(); it != nonzeroVector.end(); it++) {
        double gradient = base_grad * sample[*it];
        sigma = (sqrt(para_ns[*it] + pow(gradient, 2)) - sqrt(para_ns[*it])) / m_alpha;
        para_zs[*it] += gradient - sigma * para_weights[*it];
        para_ns[*it] += pow(gradient, 2);
    }
    sigma = (sqrt(para_ns[inputDim] + pow(base_grad, 2)) - sqrt(para_ns[inputDim])) / m_alpha;
    para_zs[inputDim] += base_grad - sigma * para_weights[inputDim];
    para_ns[inputDim] += pow(base_grad, 2);



    //get new prediction
    if (ENSEMBLE) {
        for (size_t i = 0; i < model_num; i++) {
            std::vector<double> para_weights_m = modelPool[i]["weights"];
            Predictiongroup predictiongroup_m = predict(sample, para_weights_m);
            int prediction_m = predictiongroup_m.prediction;
            double prob_m = predictiongroup_m.prob;

            if (i == current_model) {
                prediction = prediction_m;
                prediction_prob = prob_m;
                if (history_prediction.size() < SAVEHISTORY) {
                    history_prediction.push_back(prediction);
                }
                else {
                    history_prediction.erase(history_prediction.begin());
                    history_prediction.push_back(prediction);
                }
            }

            if (model_history_prediction[i].size() < SAVEHISTORY) {
                model_history_prediction[i].push_back(prediction_m);
            }
            else {
                model_history_prediction[i].erase(model_history_prediction[i].begin());
                model_history_prediction[i].push_back(prediction_m);
            }
        }
    }
    else {
        Predictiongroup predictiongroup = predict(sample, para_weights);
        prediction = predictiongroup.prediction;
        prediction_prob = predictiongroup.prob;

        if (history_prediction.size() < SAVEHISTORY) {
            history_prediction.push_back(prediction);
        }
        else {
            history_prediction.erase(history_prediction.begin());
            history_prediction.push_back(prediction);
        }
    }

    if (ENSEMBLE == true) {
        if (current_monitor_para["network_delay"][delaysize - 3] > RTT_THRESHOLD && \
            current_monitor_para["network_delay"][delaysize - 2] > RTT_THRESHOLD && \
            current_monitor_para["network_delay"][delaysize - 1] > RTT_THRESHOLD) {
            if (used_model < model_num - 1) {
                current_model += 1;
                model_accuracy.push_back(accuracy);
                used_model += 1;
            }
            else {
                for (int i = 0; i < model_num; i++) {
                    model_accuracy[i] = check_accuracy(model_history_prediction[i], history_label);
                    current_model = argmax(model_accuracy.begin(), model_accuracy.end());
                }
            }
        }
    }


    Aggregationgroup agg_group = policy_aggregation(prediction_prob);

    decision = agg_group.decision;
    decision_prob = agg_group.prob;

    if (history_decision.size() < SAVEHISTORY) {
        history_decision.push_back(decision);
    }
    else {
        history_decision.erase(history_decision.begin());
        history_decision.push_back(decision);
    }

    cwnd = rate_control(decision_prob);

    std::map<std::string, double> log_info;

    intervention_prob = compute_intervention_prob(history_prediction, history_decision);
    log_info["intervention_prob"] = intervention_prob;
    return { cwnd, utility,log_info };
}


double Monax::utility_function(std::vector<double> sample, int RTT_threshold, bool RTT_prediction) {

    double sr_weight = 0.8;
    double sr_weight_min = 0.7;
    double sr_weight_max = 0.9;
    double sr_weight_stepsize = 0.01;
    double utility, RTT_prediction_part;


    // sample = { delivery_rate, network_delay_gradient, monitor_para["network_delay"][-1]}
    double delivery_rate = sample[0];
    double network_delay_gradient = sample[1];
    double network_delay = sample[2];
    double packet_loss = 0;
    //double current_loss = sample[3];
    //double throughput_error = sample[4];

    if (this->LossFlag) {
        packet_loss = 1;
    }

    /*if (sample[4] > 1.0) {
        if (sr_weight > sr_weight_min - sr_weight_stepsize) {
            sr_weight -= sr_weight_stepsize;
        }
    }*/

    if (network_delay - RTT_threshold < -10) {
        if (sr_weight < sr_weight_max + sr_weight_stepsize) {
            sr_weight += sr_weight_stepsize;
        }
    }

    std::vector<double> weights = { sr_weight,-0.9,-0.01,-0.4,0.001,0.001 };
    double sending_rate_part = delivery_rate * weights[0];
    double RTT_g_part = weights[1] * network_delay_gradient * delivery_rate;
    double RTT_exceed_part = weights[2] * std::max(network_delay - RTT_threshold, 0.0) * delivery_rate;
    double loss_part = weights[3] * packet_loss * delivery_rate;
    double RTT_max_part = weights[2] * network_delay * delivery_rate;


    if ((network_delay - RTT_threshold) > 0) {
        utility = RTT_g_part + loss_part;
    }
    else {
        utility = sending_rate_part + loss_part;
    }

    return utility;
}

double Monax::compute_intervention_prob(std::vector<int> historyPrediction, std::vector<int> historyDecision) {

    int intervention_count = 0;
    for (size_t i = 0; i < historyPrediction.size(); i++) {
        if (historyPrediction[i] == historyDecision[i]) {
            intervention_count++;
        }
    }
    return intervention_count / historyPrediction.size();
}


Aggregationgroup Monax::policy_aggregation(double prob) {

    int aggregate_decision;
    double delta_utility = *(utility_record.end() - 1) - *(utility_record.end() - 2);

    int delaysize = current_monitor_para["network_delay"].size();
    if (current_monitor_para["network_delay"][delaysize - 1] > RTT_THRESHOLD) {
        aggregate_decision = 0;
        if (*(history_prediction.end() - 1) == aggregate_decision) {
            return { aggregate_decision,prob };
        }
        else {
            return { aggregate_decision,1 - prob };
        }
    }

    if (delta_utility > 0) {
        return { *(history_prediction.end() - 1),prob };
    }
    else {
        aggregate_decision = 1 - *(history_decision.end() - 1);
    }

    double div_threshold_scale = (RTT_THRESHOLD - (accumulate(current_monitor_para["network_delay"].end() - 3, \
        current_monitor_para["network_delay"].end(), 0.0f) / 3)) / RTT_THRESHOLD;

    if (div_threshold_scale > 0 && (rand() % 100) / 100 < div_threshold_scale) {
        aggregate_decision = 1;
    }
    if (div_threshold_scale < 0 && (rand() % 100) / 100 < (-1) * div_threshold_scale) {
        aggregate_decision = 0;
    }
    if (*(history_prediction.end() - 1) != aggregate_decision) { prob = 1 - prob; }
    return { aggregate_decision,prob };
}

double Monax::rate_control(double result) {

    int delaysize = current_monitor_para["network_delay"].size();
    double current_network_delay = current_monitor_para["network_delay"][delaysize - 1];

    int base_CWND = this->currentCWND;
    if (result < 0.5) {
        //decrease sending_rate
        if (*(history_decision.end() - 1) == 0 && *(history_decision.end() - 1) == 0 && decrease_step < stepsize_max) {
            decrease_step += decrease_step_change_rate;
        }
        else {
            decrease_step = int(decrease_step / 2);
        }
        if (base_CWND - decrease_step > CWND_MIN) {
            base_CWND -= decrease_step;
        }
        else {
            base_CWND += 3 * decrease_step;
        }
    }

    else {
        if (*(history_decision.end() - 1) == 1 && *(history_decision.end() - 1) == 0 && increase_step < stepsize_max) {
            increase_step += increase_step_change_rate;
        }
        else {
            increase_step = initial_step_size;
        }
        if (base_CWND + increase_step < CWND_MAX) {
            base_CWND += increase_step;
        }
        else {
            base_CWND -= 3 * increase_step;
        }
    }
    return base_CWND;
}


// int main() {
//     Monax CC_Monax(5, 0.2, 1.0, 0.2, 0.2, 30.0);s

//     for (int i = 0; i < 200000; i++) {
//         CC_Monax.setRealRtt(12.0);
//         CC_Monax.setDeliveryRate(30.0);
//         CC_Monax.setFlightSize(100);
//         CC_Monax.setSndPeriod(5);
//         CC_Monax.setWndSize(30); 
//         CC_Monax.setSrtLossSeqStatus(1);

//         CC_Monax.calcCCPara();
//         std::cout << "CWND = " << CC_Monax.getWndSize() << std::endl;
//         std::cout << "Period = " << CC_Monax.getSndPeriod() << std::endl;
//     }
// }