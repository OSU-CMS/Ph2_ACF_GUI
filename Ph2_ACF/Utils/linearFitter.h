/*!

        \file                   linearFitter.h
        \brief                  Base class for for fitting a straight line, without ROOT!
        \author                 Alexander Pauls
        \version                1.0
        \date                   02/02/2021
        Support :               mail to : alexander.pauls@rwth-aachen.de
        Source: W. Schenk, F. Kremer „Physikalisches Grundpraktikum“ 14. Auflage p.17ff (German)
                https://pages.mtu.edu/~fmorriso/cm3215/UncertaintySlopeInterceptOfLeastSquaresFit.pdf

 */
#include <fstream>
#include <iostream>
#include <map>
#include <math.h>
#include <numeric>
#include <sstream>
#include <string>
#include <vector>

namespace fitter
{
template <typename T>
void estimate_coef(std::vector<T> indep_var, std::vector<T> dep_var, double& B_1, double& B_0, double& B_1_error, double& B_0_error)
{
    double N = indep_var.size();

    double mean_x = std::accumulate(indep_var.begin(), indep_var.end(), 0.0) / N;
    double mean_y = std::accumulate(dep_var.begin(), dep_var.end(), 0.0) / N;

    double mean_x_square  = std::inner_product(indep_var.begin(), indep_var.end(), indep_var.begin(), 0.0) / N;
    double mean_x_times_y = std::inner_product(indep_var.begin(), indep_var.end(), dep_var.begin(), 0.0) / N;

    B_1 = (mean_x_times_y - mean_x * mean_y) / (mean_x_square - pow(mean_x, 2.));
    B_0 = (mean_x_square * mean_y - mean_x * mean_x_times_y) / (mean_x_square - pow(mean_x, 2.));
    std::cout << "Delta : " << std::endl;

    double zwischen_summe = 0;

    for(size_t index = 0; index < indep_var.size(); ++index) { zwischen_summe += pow(B_1 * indep_var[index] + B_0 - dep_var[index], 2.); }
    double DELTA = sqrt(zwischen_summe / (N - 2.));

    B_1_error = DELTA * sqrt(1. / (N * (mean_x_square - pow(mean_x, 2.))));
    B_0_error = DELTA * sqrt(mean_x_square / (N * (mean_x_square - pow(mean_x, 2.))));

    std::cout << "B1_error : " << B_1_error << std::endl;
    std::cout << "B0_error : " << B_0_error << std::endl;
    // std::cout << "mean_x : " << mean_x << std::endl;
    // std::cout << "mean_y : " << mean_y << std::endl;
    // std::cout << "mean_x_square : " << mean_x_square << std::endl;
    // std::cout << "mean_x_times_y : " << mean_x_times_y << std::endl;
}
template <typename T>
void estimate_coef(std::vector<T> indep_var, std::vector<T> dep_var, std::vector<T> errors, double& B_1, double& B_0, double& B_1_error, double& B_0_error)
{
    std::vector<double> weights;
    for(size_t index = 0; index < errors.size(); ++index)
    {
        if(errors[index] == 0) { weights.push_back(0); }
        else { weights.push_back(1. / pow(errors[index], 2)); }
    }
    double sum_weights = std::accumulate(weights.begin(), weights.end(), 0.0);
    double mean_x      = std::inner_product(indep_var.begin(), indep_var.end(), weights.begin(), 0.0) / sum_weights;
    double mean_y      = std::inner_product(dep_var.begin(), dep_var.end(), weights.begin(), 0.0) / sum_weights;

    double mean_x_square = 0;
    for(size_t index = 0; index < indep_var.size(); ++index) { mean_x_square += weights[index] * pow(indep_var[index], 2.); }
    mean_x_square /= sum_weights;
    double mean_x_times_y = 0;
    for(size_t index = 0; index < indep_var.size(); ++index) { mean_x_times_y += weights[index] * indep_var[index] * dep_var[index]; }
    mean_x_times_y /= sum_weights;
    B_1 = (mean_x_times_y - mean_x * mean_y) / (mean_x_square - pow(mean_x, 2.));
    B_0 = (mean_x_square * mean_y - mean_x * mean_x_times_y) / (mean_x_square - pow(mean_x, 2.));

    B_1_error = sqrt(1. / (sum_weights * (mean_x_square - pow(mean_x, 2.))));
    B_0_error = sqrt(mean_x_square / (sum_weights * (mean_x_square - pow(mean_x, 2.))));
    std::cout << "B1_error : " << B_1_error << std::endl;
    std::cout << "B0_error : " << B_0_error << std::endl;
    // std::cout << "mean_x : " << mean_x << std::endl;
    // std::cout << "mean_y : " << mean_y << std::endl;
    // std::cout << "mean_x_square : " << mean_x_square << std::endl;
    // std::cout << "mean_x_times_y : " << mean_x_times_y << std::endl;
}

template <typename T>
class Linear_Regression
{
  private:
  public:
    double b_0;
    double b_1;
    double b_0_error;
    double b_1_error;

    void fit(std::vector<T> datasetX, std::vector<T> datasetY) { estimate_coef<T>(datasetX, datasetY, b_1, b_0, b_1_error, b_0_error); }
    void fit(std::vector<T> datasetX, std::vector<T> datasetY, std::vector<T> datasetYerrors) { estimate_coef<T>(datasetX, datasetY, datasetYerrors, b_1, b_0, b_1_error, b_0_error); }
};

} // namespace fitter
