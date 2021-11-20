#include <ctime>
#include <iostream>
#include <limits>

#include "relax.hpp"

RelaxationLabeling::RelaxationLabeling(const Eigen::Tensor<double, 4>& compatibility) :
    compatibility(compatibility),
    supportFactor(1.0),
    iteration(0),
    iterationLimit(30),
    save(true),
    verbose(0)
{
    numObjects = compatibility.dimension(0);
    numLabels = compatibility.dimension(1);
    
    double initialStrength = 1.0/numLabels;
    strength = Eigen::MatrixXd::Constant(numObjects, numLabels, initialStrength);
    support = Eigen::MatrixXd::Constant(numObjects, numLabels, 0.0);


    std::clock_t start = std::clock();
    while (iteration < iterationLimit)
    {
        iterate();
    }
    std::clock_t stop = std::clock();
    for (size_t i = 0; i < numObjects; ++i)
    {
        std::cout << "final strength for object " << i << std::endl << strength.row(i) << std::endl;
    }
    for (size_t i = 0; i < numObjects; ++i)
    {
        int index;
        double val = strength.row(i).maxCoeff(&index);
        std::cout << "best strength for object #" << i << " is " << val << " with label " << index << std::endl;
    }
    std::cout << iterationLimit << " loops for " << numObjects << " objects and " << numLabels << " labels: " << (stop - start)/1000.0 << " millisec.  Hertz: " << 1.0/((stop-start)/1000000.0) << std::endl;
}

void RelaxationLabeling::iterate()
{
    updateSupport();
    updateStrength();
    ++iteration;
}

void RelaxationLabeling::updateSupport()
{
    std::array<long, 4> extent = {1,1,numObjects,numLabels};  // Size of array to slice from starting point

    for (size_t i = 0; i < numObjects; ++i)
    {
        for (size_t j = 0; j < numLabels; ++j)
        {
            std::array<long, 4> offset = {long(i),long(j),0,0};  // Starting point
            Eigen::Tensor<double, 4> compatSliceTensor = compatibility.slice(offset, extent);

            Eigen::MatrixXd compatSlice = Eigen::Map<const Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic>> (compatSliceTensor.data(), numObjects, numLabels);

            support(i,j) = (compatSlice.array()*strength.array()).sum();
        }
        if (verbose > 1)
        {
            std::cout << "raw support for object " << i << std::endl << support.row(i) << std::endl;
        }
    }
    normalizeSupport(SupportNormalizationMethod::ALL_IJ);
}

void RelaxationLabeling::normalizeSupport(SupportNormalizationMethod snm)
{
    if (snm == SupportNormalizationMethod::ALL_IJ)
    {
        double minElement = support.minCoeff();
        double maxElement = support.maxCoeff();
        support = (support.array() - minElement)/maxElement;
    }
}

void RelaxationLabeling::updateStrength()
{
    strength = strength.array() + strength.array()*support.array()*supportFactor;
    strength.rowwise().normalize();
}

void RelaxationLabeling::normalizeStrength(size_t i, double sumStrengths)
{
    for (size_t j = 0; j < numLabels; ++j)
    {
        strength(i, j) /= sumStrengths;
    }
}

void RelaxationLabeling::assign()
{
    
}