#ifndef COUT_CAPTURE_HPP
#define COUT_CAPTURE_HPP

#include <iostream>
#include <sstream>
#include <streambuf>

class CoutCapture {
public:
    CoutCapture() : oldBuf_(std::cout.rdbuf(capture_.rdbuf())) {}

    ~CoutCapture() { std::cout.rdbuf(oldBuf_); }

    std::string str() const { return capture_.str(); }

    void clear() { capture_.str(""); capture_.clear(); }

private:
    std::ostringstream capture_;
    std::streambuf* oldBuf_;
};

#endif
