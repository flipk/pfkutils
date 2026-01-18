#ifndef __AVERAGING_ALGORITHMS_H__
#define __AVERAGING_ALGORITHMS_H__ 1

#include <vector>
#include <inttypes.h>

typedef enum {
    AVG_ALG_DEGLITCH = 1,
    AVG_ALG_SMA = 2,
    AVG_ALG_EMA = 3
} Averaging_Algorithm_t;

class AverageAlg
{
    Averaging_Algorithm_t m_type;
protected:
    bool m_valid;
    bool m_configured;
    void configured(void) { m_valid = false; m_configured = true; }
public:
    AverageAlg(Averaging_Algorithm_t type)
        : m_type(type), m_valid(false), m_configured(false) { }
    Averaging_Algorithm_t get_type(void) const { return m_type; }
    virtual ~AverageAlg(void) { }
    virtual void reinit(void) = 0;
    virtual double add_reading(double v) = 0;
    bool valid(void) const { return m_configured && m_valid; }
};

// deglitch buffer (min of circular history)
class Deglitcher : public AverageAlg
{
    std::vector<double> m_history;
    int m_pos;
public:
    Deglitcher(void)
        : AverageAlg(AVG_ALG_DEGLITCH), m_pos(0) { }
    ~Deglitcher(void) { }
    void configure(uint32_t history_size) {
        if (history_size > 0) {
            m_history.resize(history_size);
            reinit();
            configured();
        }
    }
    /*virtual*/ void reinit(void) {
        m_valid = false;
        for (int ind = 0; ind < m_history.size(); ind++)
            m_history[ind] = 99;
        m_pos = 0;
    }
    /*virtual*/ double add_reading(double v) {
        if (!m_configured)
            return v;
        m_history[m_pos] = v;
        if (++m_pos >= m_history.size())
        {
            m_valid = true;
            m_pos = 0;
        }
        double min = 99;
        for (int ind = 0; ind < m_history.size(); ind++)
        {
            double v = m_history[ind];
            if (v < min)
                min = v;
        }
        return min;
    }
};

// simple moving average (average of circular history)
class AverageAlgSMA : public AverageAlg
{
    std::vector<double> m_history;
    double m_sma_sum;
    int m_num_stored;
    int m_pos;
public:
    AverageAlgSMA(void)
        : AverageAlg(AVG_ALG_SMA),
          m_sma_sum(0.0), m_num_stored(0), m_pos(0) { }
    ~AverageAlgSMA(void) { }
    void configure(uint32_t history_size) {
        m_history.resize(history_size);
        reinit();
        configured();
    }
    /*virtual*/ void reinit(void) {
        m_valid = false;
        m_sma_sum = 0.0;
        for (int ind = 0; ind < m_history.size(); ind++)
            m_history[ind] = 0.0;
        m_num_stored = 0;
        m_pos = 0;
    }
    /*virtual*/ double add_reading(double v) {
        if (!m_configured)
            return -99;
        m_sma_sum -= m_history[m_pos];
        m_history[m_pos] = v;
        m_sma_sum += v;
        if (++m_pos >= m_history.size())
            m_pos = 0;
        if (m_num_stored < m_history.size())
            m_num_stored++;
        if (m_num_stored == m_history.size())
            m_valid = true;
        return m_sma_sum / m_num_stored;
    }
};

// exponential moving average
class AverageAlgEMA : public AverageAlg
{
    double m_alpha;
    double m_1malpha; // 1-alpha
    double m_history;
public:
    AverageAlgEMA(void) : AverageAlg(AVG_ALG_EMA) { }
    ~AverageAlgEMA(void) { }
    static double calc_alpha(double N) {
        return 2.0 / (1.0 + N);
    }
    void configure(double alpha) {
        m_alpha = alpha;
        m_1malpha = 1.0 - alpha;
        m_history = 0.0;
        configured();
    }
    /*virtual*/ void reinit(void) {
        m_valid = false;
        m_history = 0.0;
    }
    /*virtual*/ double add_reading(double v) {
        if (!m_configured)
            return -99;
        if (!m_valid) {
            m_valid = true;
            m_history = v;
        } else
            m_history = (m_alpha * v) + (m_1malpha * m_history);
        return m_history;
    }
};

#endif // __AVERAGING_ALGORITHMS_H__
