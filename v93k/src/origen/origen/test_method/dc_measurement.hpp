#ifndef ORIGEN_TEST_METHOD_DC_MEASUREMENT_INCLUDED
#define ORIGEN_TEST_METHOD_DC_MEASUREMENT_INCLUDED

#include "base.hpp"
#include "mapi.hpp"
#include "rdi.hpp"

using namespace std;

namespace Origen {
namespace TestMethod {

class DCMeasurement: public Base {

    int _applyShutdown;
    string _shutdownPattern;
    string _measure;
    double _settlingTime;
    string _pin;
    double _forceValue;


public:
    virtual ~DCMeasurement() { };
    void SMC_backgroundProcessing();
    void execute();

    DCMeasurement & applyShutdown(int v) { _applyShutdown = v; return *this; }
    DCMeasurement & measure(string v) { _measure = v; return *this; }
    DCMeasurement & settlingTime(double v) { _settlingTime = v; return *this; }
    DCMeasurement & pin(string v) { _pin = v; return *this; }
    DCMeasurement & forceValue(double v) { _forceValue = v; return *this; }

protected:
    // All test methods must implement this function
    DCMeasurement & getThis() { return *this; }

    // Internal variables, declared outside the the execute function body since
    // they may be useful to refer to in callback functions
    ARRAY_I activeSites;
    string testSuiteName;
    string label;
    vector<int> results;
};

void DCMeasurement::execute() {

    int site, physicalSites;
    ARRAY_I sites;

    RDI_INIT();

    ON_FIRST_INVOCATION_BEGIN();

    label = Primary.getLabel();
    // Enable Smart Calc
    rdi.hiddenUpload(TA::ALL);
    GET_ACTIVE_SITES(activeSites);
    physicalSites = GET_CONFIGURED_SITES(sites);
    results.resize(physicalSites + 1);
    GET_TESTSUITE_NAME(testSuiteName);
    label = Primary.getLabel();

    RDI_BEGIN();

    if (preTestFunc()) {
        rdi.func("f1").label(label).execute();
    }

    RDI_END();

    postTestFunc();

    FOR_EACH_SITE_BEGIN();
    site = CURRENT_SITE_NUMBER();
    results[site] = rdi.id("f1").getPassFail();
    FOR_EACH_SITE_END();

    if (preProcessFunc()) {
        SMC_ARM();
    } else {
        processFunc();
        postProcessFunc();
    }

    ON_FIRST_INVOCATION_END()
    ;

}

void DCMeasurement::SMC_backgroundProcessing() {
    if (processFunc()) {
        for (int i = 0; i < activeSites.size(); i++) {
            SMC_TEST("", testSuiteName, LIMIT(TM::GE, 1, TM::LE, 1), results[activeSites[i]]);
        }
    }
    postProcessFunc();
}

}
}
#endif
