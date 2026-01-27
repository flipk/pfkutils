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
    double m_sma_partial_sum;
    int m_num_stored;
    int m_pos;
public:
    AverageAlgSMA(void)
        : AverageAlg(AVG_ALG_SMA),
          m_sma_sum(0.0), m_sma_partial_sum(0.0),
          m_num_stored(0), m_pos(0) { }
    ~AverageAlgSMA(void) { }
    void configure(uint32_t history_size) {
        m_history.resize(history_size);
        reinit();
        configured();
    }
    /*virtual*/ void reinit(void) {
        m_valid = false;
        m_sma_sum = 0.0;
        m_sma_partial_sum = 0.0;
        for (int ind = 0; ind < m_history.size(); ind++)
            m_history[ind] = 0.0;
        m_num_stored = 0;
        m_pos = 0;
    }
    /*virtual*/ double add_reading(double v) {
        if (!m_configured)
            return -99;

        // to be efficient, we don't want to have to recalculate the
        // sum of the whole history buffer on EVERY new input.  if the
        // SMA history is big, that could be rather a lot of work.  a
        // way to change an O(n) algorithm to an O(1) algorithm is to
        // keep a running sum. every time we overwrite an entry in the
        // history, subtract the old value and add the new value.
        // that way, there's one subtract, one add, and one divide,
        // for every new input.
        //
        // BUT! a running sum can drift over time due to rounding
        // errors at the least significant bit. e.g. if you're storing
        // numbers in the range 0 to 100, bit 53 of the mantissa
        // roughly corresponds to decimal 0.000000000000014.
        // so when there is a round error at bit 53:
        //            (A + B) - A != B
        //
        // assuming every operation rounds in the same direction,
        // after a few billion iterations, the error accumulates up to
        // the integer level. in reality not every operation will round
        // in the same direction, but still, it could.
        // if your app will take e.g. ten years to accumulate a few
        // billion iterations, you won't care about this.  but if your
        // app will reach a few billion in an hour, or a day, you will
        // definitely care about this.
        //
        // SOLUTION: periodic re-summation. keep two running sums. one
        // is the full running sum of the past N samples, circularly,
        // doing the subtract-and-add trick as described above; the
        // other sum is a partial sum, which starts at 0 every time we
        // pass entry 0 of the history.  every time we get to the last
        // entry, that partial sum now represents the sum of every
        // entry now in the history. at that moment, replace the
        // running sum with the partial sum, and reset the partial sum
        // back to 0. if there were no rounding errors at any step,
        // these two numbers are identical, but if there were, the
        // partial sum will reset it back to correct.  this is still
        // O(1), just with two adds instead of one (and one subtract
        // and one divide) for every input, but has no drift.

        double old_value = m_history[m_pos];
        m_history[m_pos] = v;
        m_sma_partial_sum += v;

        if (++m_pos >= m_history.size())
        {
            m_pos = 0;
            // reinit running sum
            m_sma_sum = m_sma_partial_sum;
            // init partial sum
            m_sma_partial_sum = 0;
        }
        else
        {
            m_sma_sum -= old_value;
            m_sma_sum += v;
        }

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
