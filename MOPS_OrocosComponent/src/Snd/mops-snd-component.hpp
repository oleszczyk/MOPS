#ifndef OROCOS_MOPS_SND_COMPONENT_HPP
#define OROCOS_MOPS_SND_COMPONENT_HPP

#include <rtt/RTT.hpp>
#include <string>

extern "C"
{
#include "MOPS.h"
}

struct MyData{
  char data[MAX_MESSAGE_LENGTH];
};

class Mops_Snd : public RTT::TaskContext{
  public:
    Mops_Snd(std::string const& name);
    bool configureHook();
    bool startHook();
    void updateHook();
    void stopHook();
    void cleanupHook();
  private:
    bool _verbose;
    int _priority;
    unsigned _cpu;
    unsigned _timeout;   // [ms]
    std::string _topicName;
    RTT::InputPort<MyData>  _inPort;
};
#endif
