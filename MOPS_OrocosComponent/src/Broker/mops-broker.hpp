#ifndef OROCOS_MOPS_BROKER_COMPONENT_HPP
#define OROCOS_MOPS_BROKER_COMPONENT_HPP

#include <rtt/RTT.hpp>
#include <string>

class Mops_Broker : public RTT::TaskContext{
  public:
    Mops_Broker(std::string const& name);
    bool configureHook();
    bool startHook();
    void updateHook();
    void stopHook();
    void cleanupHook();
  private:
    bool _run;
    bool _verbose;
    int _priority;
    unsigned _cpu;
    unsigned _timeout;   // [ms]
    RTT::OutputPort<std::string>  _outPort;
};
#endif
