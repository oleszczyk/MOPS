#ifndef OROCOS_MOPS_REC_COMPONENT_HPP
#define OROCOS_MOPS_REC_COMPONENT_HPP

#include <rtt/RTT.hpp>
#include <string>

extern "C"
{
#include "MOPS.h"
}

struct MyData{
  char data[MAX_MESSAGE_LENGTH];
};

class Mops_Rec : public RTT::TaskContext{
  public:
    Mops_Rec(std::string const& name);
    bool configureHook();
    bool startHook();
    void updateHook();
    void stopHook();
    void cleanupHook();
  private:
    bool _verbose;
    int _priority;
    unsigned _cpu;
    RTT::OutputPort<MyData>  _outPort;
    
    void _addSub(std::string topic);
};
#endif
