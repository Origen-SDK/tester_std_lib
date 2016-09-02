#ifndef ORIGEN_TEST_METHOD_FREQUENCY_MEASUREMENT_INCLUDED
#define ORIGEN_TEST_METHOD_FREQUENCY_MEASUREMENT_INCLUDED

#include "base.hpp"
#include "mapi.hpp"
#include "rdi.hpp"

using namespace std;

namespace Origen {
namespace TestMethod {

class FrequencyMeasurement: public Base  {
    void serialProcessing(int site);

    int _periodBased;
    string _pin;
    int _samples;
    int _periodInNs;

public:
    // Defaults
    FrequencyMeasurement() {
        samples(2000);
    }
    virtual ~FrequencyMeasurement() { }
    void SMC_backgroundProcessing();
    void execute();

    /// By default the limits are assumed to be expressed as a frequency, set this to 1 if they refer to a target period
    FrequencyMeasurement & periodBased(int v) { _periodBased = v; return *this; }
    /// REQUIRED: The name of the pin being measured
    FrequencyMeasurement & pin(string v) { _pin = v; return *this; }
    /// The number of samples captured by the pattern, the default is 2000
    FrequencyMeasurement & samples(int v) { _samples = v; return *this; }
    /// REQUIRED: Supply the period of the captured vectors in nanoseconds
    FrequencyMeasurement & periodInNs(int v) { _periodInNs = v; return *this; }

protected:
    // All test methods must implement these functions
    FrequencyMeasurement & getThis() { return *this; }

    // Internal variables, declared outside the the execute function body since
    // they may be useful in callback functions
    ARRAY_I activeSites;
    string testSuiteName;
    string label;
    vector<int> funcResults;
    vector<ARRAY_I> results;
    LIMIT limits;
};

void FrequencyMeasurement::execute() {

    int site, physicalSites;
    ARRAY_I sites;

    RDI_INIT();

    ON_FIRST_INVOCATION_BEGIN();

    enableHiddenUpload();
    GET_ACTIVE_SITES(activeSites);
    physicalSites = GET_CONFIGURED_SITES(sites);
    results.resize(physicalSites + 1);
    funcResults.resize(physicalSites + 1);
    GET_TESTSUITE_NAME(testSuiteName);
    label = Primary.getLabel();

    pin(extractPinsFromGroup(_pin));
    limits = GET_LIMIT_OBJECT("Functional");

    RDI_BEGIN();

    if (preTestFunc()) {
        rdi.digCap(testSuiteName)
		   .label(label)
		   .pin(_pin)
		   .bitPerWord(1)
		   .samples(_samples)
		   .execute();
    }

    RDI_END();

    postTestFunc();

    FOR_EACH_SITE_BEGIN();
        site = CURRENT_SITE_NUMBER();
        funcResults[site] = rdi.id(testSuiteName).getPassFail();
        // TODO: This retrieval needs to move to the SMC func in the async case
        results[site] = rdi.id(testSuiteName).getVector(_pin);
    FOR_EACH_SITE_END();

    asyncProcessing(this);

    ON_FIRST_INVOCATION_END();

    finalProcessing();

}

void FrequencyMeasurement::serialProcessing(int site) {
    double result;
    if (_periodBased) {
        result = calculateFrequency(results[site], _periodInNs);
    } else {
        result = calculatePeriod(results[site], _periodInNs);
    }

    TESTSET().judgeAndLog_FunctionalTest(funcResults[site]);
    TESTSET().judgeAndLog_ParametricTest(_pin, testSuiteName, limits, result);
}

void FrequencyMeasurement::SMC_backgroundProcessing() {
    double result;
    if (processFunc()) {
        for (int i = 0; i < activeSites.size(); i++) {
            int site = activeSites[i];
            if (_periodBased) {
                result = calculateFrequency(results[site], _periodInNs);
            } else {
                result = calculatePeriod(results[site], _periodInNs);
            }
            SMC_TEST(site, "", testSuiteName, LIMIT(TM::GE, 1, TM::LE, 1), funcResults[site]);
            SMC_TEST(site, _pin, testSuiteName, limits, result);
        }
    }
    postProcessFunc();
}

}
}
#endif
