#ifndef CPPUNIT_H
#define CPPUNIT_H

// Required headers, or just use #include <bits/stdc++.h>
#include <iostream>
#include <sstream>
#include <cstring>
#include <string>
#include <ctime>
#include <cmath>


// CPlusPlusUnit - C++ Unit testing TDD framework (github.com/cppunit/cppunit)
class Cppunit {

  private:
    static float machineFloatEpsilon() {
      float x = 1.0f, res;
      do {
          res = x;
      } while (1.0f + (x /= 2.0f) > 1.0f);
      return res;
    }

    static double machineDoubleEpsilon() {
      double x = 1.0, res;
      do {
          res = x;
      } while (1.0 + (x /= 2.0) > 1.0);
      return res;
    }

  public:

    #define PRINTM(m)           print(m, __FILE__, __LINE__, __FUNCTION__);
    #define CHECK(a,b)          check<long long>("", a, b, #a, #b, __FILE__, __LINE__, __FUNCTION__);
    #define CHECKM(m,a,b)       check<long long>(m, a, b, #a, #b, __FILE__, __LINE__, __FUNCTION__);
    #define CHECKD(m,a,b)       checkDelta<double>(m, a, b, doubleEpsilon, #a, #b, __FILE__, __LINE__, __FUNCTION__);
    #define CHECKDD(m,a,b,c)    checkDelta<double>(m, a, b, c, #a, #b, __FILE__, __LINE__, __FUNCTION__);
    #define CHECKT(a)           check<bool>("", a, true, #a, "true", __FILE__, __LINE__, __FUNCTION__);
    #define CHECKTM(m,a)        check<bool>(m, a, true, #a, "true", __FILE__, __LINE__, __FUNCTION__);
    #define CHECKS(a,b)         check<cs>("", a, b, #a, #b, __FILE__, __LINE__, __FUNCTION__);
    #define CHECKSM(m,a,b)      check<cs>(m, a, b, #a, #b, __FILE__, __LINE__, __FUNCTION__);

    typedef const std::string& cs;

    int checks, fails; std::ostringstream serr; std::istringstream *in;
    float floatEpsilon;
    double doubleEpsilon;

    Cppunit()
    : checks(0), fails(0), floatEpsilon(machineFloatEpsilon()), doubleEpsilon(machineDoubleEpsilon())
    {}

    virtual ~Cppunit() {}

    void test_cin(cs s){ in = new std::istringstream(s); std::cin.rdbuf(in->rdbuf()); }

    void fail_hdr(cs stra, cs strb, cs file, int line, cs func) {
        serr << "==================================================" << std::endl;
        serr << "FAIL: " << func << std::endl;
        serr << "--------------------------------------------------" << std::endl;
        serr << "File \"" << file << "\", line " << line << " in " << func << std::endl;
        serr << "  Checking " << stra << " == " << strb << std::endl;
    }

    void print(cs m, cs file, int line, cs func) {
        std::cerr << std::endl << m << "; file \"" << file << "\", line " << line << " in " << func << std::endl;
    }

    template <typename T> void check(cs m, T a, T b, cs stra, cs strb, cs file, int line, cs func) {
        checks++; if (a == b) { std::cout << "."; return; }
        fails++; std::cout << "F"; fail_hdr(stra, strb, file, line, func);
        serr << "  Error: " << m << ": \"" << a << "\" ! = \"" << b << "\"" << std::endl << std::endl;
    }

    template <typename T> void checkDelta(cs m, T a, T b, T d, cs stra, cs strb, cs file, int line, cs func) {
        checks++; if ( labs ( a - b ) < d ) { std::cout << "."; return; }
        fails++; std::cout << "F"; fail_hdr(stra, strb, file, line, func);
        serr << "  Error: " << m << ": \"" << a << "\" ! = \"" << b << "\" (delta " << d << ")" << std::endl << std::endl;
    }

    virtual void single_test() {}
    virtual void test_list() { single_test(); }
    double dclock() { return double(clock()) / CLOCKS_PER_SEC; }
    int status() {
        std::cout << std::endl; if (fails) std::cout << serr.str();
        std::cout << "--------------------------------------------------" << std::endl;
        std::cout << "Ran " << checks << " checks in " << dclock() << "s" << std::endl << std::endl;
        if (fails) std::cout << "FAILED (failures=" << fails << ")"; else std::cout << "OK" << std::endl;
        return fails > 0;
    }
    int run() { std::streambuf* ocin = std::cin.rdbuf(); test_list(); std::cin.rdbuf(ocin); return status(); }
};

template<> void Cppunit::checkDelta<float>(cs m, float a, float b, float epsilon, cs stra, cs strb, cs file, int line, cs func) {
    checks++; if ( fabsf( a - b ) < epsilon ) { std::cout << "."; return; }
    fails++; std::cout << "F"; fail_hdr(stra, strb, file, line, func);
    serr << "  Error: " << m << ": \"" << a << "\" ! = \"" << b << "\" (epsilon " << epsilon << ")" << std::endl << std::endl;
}

template<> void Cppunit::checkDelta<double>(cs m, double a, double b, double epsilon, cs stra, cs strb, cs file, int line, cs func) {
    checks++; if ( fabsf( a - b ) < epsilon ) { std::cout << "."; return; }
    fails++; std::cout << "F"; fail_hdr(stra, strb, file, line, func);
    serr << "  Error: " << m << ": \"" << a << "\" ! = \"" << b << "\" (epsilon " << epsilon << ")" << std::endl << std::endl;
}


#endif // CPPUNIT_H

