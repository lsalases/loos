/*
  TimeSeries.hpp
  (c) 2008 Alan Grossfield


  Grossfield Lab
  Department of Biochemistry and Biophysics
  University of Rochester Medical School

  Basic TimeSeries/vector math class
*/

#if !defined(TIMESERIES_HPP)
#define TIMESERIES_HPP

#include <vector>
#include <stdexcept>
#include <math.h>


using namespace std;

//! Time Series Class
/*!
 *  This class provides basic operations on a time series, such
 *  as averaging, standard deviation, etc
 *
 *  One can do standard arimethic operations on time series combined with
 *  scalars or other timeseries (as long as the two time series are the same
 *  length).
 *
 */

template<class T>
class tTimeSeries {
public:
    tTimeSeries() {
        init();
    }

    tTimeSeries(const vector<T> &inp) {
        _data = inp; 
    }

    tTimeSeries(const int size, const T *array) {
        _data.reserve(size);
        for (int i=0; i<size; i++) {
            _data.push_back(array[i]);
        }
    }

    tTimeSeries(const tTimeSeries<T> &inp) {
        _data = inp._data;
    }


    void init(void) {
        _data.clear();
    }

    void zero(void) {
        _data.assign(_data.size, (T)0.0);
    }

    T& operator[](const int i) {
        return (_data.at(i)); // this way we get range checking
    }

    const T& operator[](const int i) const  {
        return (_data.at(i)); // this way we get range checking
    }

    int size(void) const {
        return (_data.size());
    }

    tTimeSeries<T> operator+=(const T val) {
        for (int i=0; i<_data.size(); i++) {
            _data[i] += val;
        }
    return(*this);
    }

    tTimeSeries<T> operator+=(const tTimeSeries<T> &rhs) {
        if (_data.size() != rhs.size())
            throw(runtime_error("mismatched timeseries sizes in +="));
        for (int i=0; i<_data.size(); i++) {
            _data[i] += rhs[i];
        }
    return(*this);
    }

    tTimeSeries<T> operator+(const T val) const {
        tTimeSeries<T> res(*this);
        for (int i=0; i<_data.size(); i++) {
            res[i] += val;
        }
    return(res);
    }


    friend tTimeSeries<T> operator+(const T lhs, const tTimeSeries<T> &rhs) {
        tTimeSeries<T> res( rhs.size() );
        for (int i=0; i<rhs.size(); i++) {
            res[i] = lhs + rhs[i];
        }
    return(res);
    }

    tTimeSeries<T> operator+(const tTimeSeries<T> &rhs) const {
        if (_data.size() != rhs.size())
            throw(runtime_error("mismatched timeseries sizes in +="));

        tTimeSeries<T> res(*this);
        for (int i=0; i<res.size(); i++) {
            res[i] += rhs[i];
        }
    return(res);
    }

    tTimeSeries<T> operator-=(const T val) {
        for (int i=0; i<_data.size(); i++) {
            _data[i] -= val;
        }
    return(*this);
    }

    tTimeSeries<T> operator-=(const tTimeSeries<T> &rhs) {
        if (_data.size() != rhs.size())
            throw(runtime_error("mismatched sizes of time series"));
        for (int i=0; i<_data.size(); i++) {
            _data[i] -= rhs[i];
        }
    return(*this);
    }

    tTimeSeries<T> operator-(const T val) const {
        tTimeSeries<T> res(*this);
        for (int i=0; i<_data.size(); i++) {
            res[i] -= val;
        }
    return(res);
    }

    tTimeSeries<T> operator-(const tTimeSeries<T> &rhs) const {
        if (_data.size() != rhs.size())
            throw(runtime_error("mismatched timeseries sizes in +="));

        tTimeSeries<T> res(*this);
        for (int i=0; i<res.size(); i++) {
            res[i] -= rhs[i];
        }
    return(res);
    }



    tTimeSeries<T> operator-() const {
        tTimeSeries<T> res(*this);
        for (int i=0; i<_data.size(); i++) {
            res[i] = -res[i];
        }
    return(res);
    }

    friend tTimeSeries<T> operator-(const T lhs, const tTimeSeries<T> &rhs) {
        tTimeSeries<T> res( rhs.size() );
        for (int i=0; i<rhs.size(); i++) {
            res[i] = lhs - rhs[i];
        }
    return(res);
    }

    tTimeSeries<T> operator*=(const T val) {
        for (int i=0; i<_data.size(); i++) {
            _data[i] *= val;
        }
    return(*this);
    }

    tTimeSeries<T> operator*(const T val) const {
        tTimeSeries<T> res(*this);
        for (int i=0; i<_data.size(); i++) {
            res[i] *= val;
        }
    return(res);
    }

    tTimeSeries<T> operator*=(const tTimeSeries<T> &rhs) {
        if ( _data.size() != rhs.size() ) 
            throw(runtime_error("mismatched timeseries sizes in *="));
        for (int i=0; i<_data.size(); i++) {
            _data[i] *= rhs[i];
        }
    return(*this);
    }

    tTimeSeries<T> operator*(const tTimeSeries<T> &rhs) const {
        if ( _data.size() != rhs.size() ) 
            throw(runtime_error("mismatched timeseries sizes in *="));

        tTimeSeries<T> res(*this);
        for (int i=0; i<_data.size(); i++) {
            res[i] *= rhs[i];
        }
    return(res);
    }

    friend tTimeSeries<T> operator*(const T lhs, const tTimeSeries<T> &rhs) {
        tTimeSeries<T> res( rhs.size() );
        for (int i=0; i<rhs.size(); i++) {
            res[i] = lhs * rhs[i];
        }
    return(res);
    }

    //! Return average of time series
    T average(void) const {
        T ave = 0.0;
        for (int i=0; i < _data.size(); i++) {
            ave += _data[i];
        }
        ave /= _data.size();
        return (ave);
    }

    //! Return variance of time series.
    T variance(void) const {
        T ave = 0.0;
        T ave2 = 0.0;
        for (int i=0; i < _data.size(); i++) {
            ave += _data[i];
            ave2 += _data[i] * _data[i];
        }

        ave /= _data.size();
        ave2 /= _data.size();
        return (ave2 - ave*ave);
    }

    //! Return standard deviation of time series.
    T stdev(void) const {
        return (sqrt(variance() ));
    }

    //! Return standard error of time series.
    //! This assumes all points are statistically independent
    //! Otherwise, needs to be multiplied by the square root of the 
    //! correlation time, in units of the step interval for the time series
    T sterr(void) const {
        return (stdev() / sqrt(_data.size()));  
    }

    //! Return a new timeseries of the same size as the current one,
    //! containing the running average of the time series
    tTimeSeries<T> running_average(void) const {
        tTimeSeries result(_data.size() );
        T sum = 0.0;
        for (int i=0; i<_data.size(); i++) {
            sum += _data[i];
            result[i] = sum / (i+1);
        }
        return(result);

    }

    //! Return a new timeseries containing the windowed average.
    //! ith value of the new time series =  1/window * sum(data[i:i+window]).
    //! NOTE: The present algorithm is relatively fast, but can be prone
    //! to roundoff.
    tTimeSeries<T> windowed_average(const int window) const {

        if (window > _data.size() )
            throw(out_of_range("Error in windowed_average: window too large"));

        tTimeSeries result(_data.size() - window);
        T sum = 0;
        for (int i=0; i<window; i++) {
            sum += _data[i];
        }
        result[0] = sum / window;


        for (int i=1; i < result.size(); i++) {
            sum = sum - _data[i-1] + _data[i+window-1];
            result[i] = sum / window;
        }
    }

    //! Return the variance of the block average for the time series.
    //! Divides the timeseries into num_blocks equally sized blocks
    //! (discarding the remaining blocks at the end), computes the average
    //! for each block, and returns the variance of the averages.
    //! This is useful for doing Flyvjberg and Jensen-style block averaging.
    //! This should probably move to a TimeSeries.cpp
    T block_var(const int num_blocks) const {
        int points_per_block = size() / num_blocks;
        T block_ave = 0.0;
        T block_ave2 = 0.0;
        for (int i=0; i<num_blocks; i++) {
            T block_sum = 0.0;
            int offset = i*points_per_block;
            for (int j=0; j< points_per_block; j++) {
                block_sum += _data[offset + j];
            }
            T ave = block_sum / points_per_block;
            block_ave += ave;
            block_ave2 += ave*ave;
        }
        block_ave /= num_blocks;
        block_ave2 /= num_blocks;
        return (block_ave2 - block_ave*block_ave);
    }
    
private:
    vector<T> _data;

    // private constructor which just sizes the data array
    // useful for running_average and windowed_average
    tTimeSeries(const int n) : _data(n) {}
};

typedef tTimeSeries<double> TimeSeries;
typedef tTimeSeries<float> fTimeSeries;


#endif
