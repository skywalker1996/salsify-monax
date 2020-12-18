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

struct Aggregationgroup {
    int decision;
    double prob;
};
struct Actiongroup {
    int cwnd;
    double utility;
    std::map< std::string, double>logInfo;
};
struct Predictiongroup {
    int prediction;
    double prob;
};

class Monax {

public:
    Monax(int dim, double alpha, double beta, double lambda1, double lambda2, double initial_cwnd);
    double utility_function(std::vector<double> sample, int RTT_threshold, bool RTT_prediction);

    Actiongroup action(std::map< std::string, std::vector<double>> monitor_para);
    Aggregationgroup policy_aggregation(double prob);

    double compute_intervention_prob(std::vector<int> historyPrediction, std::vector<int> historyDecision);
    double rate_control(double result);

    //����RTT
    void setRealRtt(double RealRtt);

    //����inflight����
    void setFlightSize(int flightSize);

    //���ô�������
    void setDeliveryRate(double m_iDeliveryRate);

    //���÷��Ͱ������m_CongCtl->pktSndPeriod_us())
    void setSndPeriod(int SndPeriod);

    //���÷��ʹ��ڣ�m_CongCtl->cgWindowSize()��
    void setWndSize(int WndSize);

    //���ö���״̬��־λ
    void setSrtLossSeqStatus(bool LossFlag);

    //����ӵ�����Ʋ��������ʹ��ںͷ��Ͱ������
    void calcCCPara();

    //��ȡ���Ͱ����
    int getSndPeriod();

    //��ȡ���ڴ�С
    int getWndSize();


private:
    double RealRtt;
    int flightSize;
    double m_iDeliveryRate;
    int SndPeriod;
    bool LossFlag;
    double currentUtility;

    std::vector<double> RttHistory;

    int inputDim, currentCWND;
    double m_alpha, m_beta, m_lambda1, m_lambda2;

    double delivery_rate, network_delay_gradient, current_loss, max_network_delay;
    int model_num;
    size_t current_model;
    std::map<int, std::map<std::string, std::vector<double>>> modelPool;
    std::vector<double> utility_record;

    //history_prediction is the output the FTRL model
    //history_decision is the final output of the aggregation module
    std::vector<int> history_prediction, history_decision, history_label;
    size_t save_history;

    //CWND step size params
    int increase_step;
    int decrease_step;
    int initial_step_size;
    int increase_step_change_rate;
    int decrease_step_change_rate;
    int stepsize_max;

    //aggregation module intervention level (0.0 - 1.0)
    int intervention_prob;
    int accuracy;
    int used_model;

    std::vector<double> model_accuracy;
    std::vector<std::vector<int> > model_history_prediction;
    std::map<std::string, std::vector<double>> current_monitor_para;
    bool coldstart;
};
