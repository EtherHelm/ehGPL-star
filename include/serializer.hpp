#pragma once
#include "shm.hpp"
#include <Eigen/Dense>
using namespace Eigen;

#include <iostream>

class shmReader
{
public:
    size_t size = 0;
    shmReader(const char *shm_name, size_t shm_size)
        : shm(shm_name, shm_size, false)
    {
        shm.read(&size, sizeof(size));
    }
    void writeResult(VectorXf &result)
    {
        shm.resetOffset();
        shm.write(result.data(), result.size() * sizeof(float));
    }
    void writeResult(MatrixXf &result)
    {
        shm.resetOffset();
        shm.write(result.data(), result.size() * sizeof(float));
    }
    template <typename T>
    T read();

private:
    SharedMemory shm;
};
template <typename T>
T shmReader::read()
{
    T val;
    shm.read(&val, sizeof(T));
    return val;
}
template <>
VectorXi shmReader::read<VectorXi>()
{
    VectorXi vec(size);
    shm.read(vec.data(), size * sizeof(int));
    return vec;
}
template <>
VectorXf shmReader::read<VectorXf>()
{
    VectorXf vec(size);
    shm.read(vec.data(), size * sizeof(float));
    return vec;
}
template <>
VectorXd shmReader::read<VectorXd>()
{
    VectorXd vec(size);
    shm.read(vec.data(), size * sizeof(double));
    return vec;
}
template <>
MatrixXf shmReader::read<MatrixXf>()
{
    MatrixXf mat(3, size);
    shm.read(mat.data(), 3 * size * sizeof(float));
    return mat;
}

template <>
MatrixXd shmReader::read<MatrixXd>()
{
    MatrixXd mat(3, size);
    shm.read(mat.data(), 3 * size * sizeof(double));
    return mat;
}

