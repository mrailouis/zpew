#pragma once

#include <stdexcept>

namespace zpew {

class Error : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class DataError : public Error {
public:
    using Error::Error;
};

class ValidationError : public Error {
public:
    using Error::Error;
};

class InvalidArgument : public Error {
public:
    using Error::Error;
};

class ConvergenceError : public Error {
public:
    using Error::Error;
};

}
