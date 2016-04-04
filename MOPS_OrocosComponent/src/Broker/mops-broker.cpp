#include "mops-broker.hpp"
#include <rtt/Component.hpp>
#include <iostream>
#include <string>

extern "C"
{
#include "MOPS.h"
}

static const int         defPriority = 10;
static const unsigned    defCpu      = 0;
static const unsigned    defTimeout  = 5000; // [ms]

Mops_Broker::Mops_Broker(std::string const& name) :
  TaskContext(name, PreOperational),
  _run(false),
  _verbose(false),
  _priority(defPriority),
  _cpu(defCpu),
  _timeout(defTimeout)
{
  // Add attributes 
  addAttribute("verbose",  _verbose);
  addAttribute("priority", _priority);
  addAttribute("cpu",      _cpu);
  addAttribute("timeout",  _timeout);

  // Add output ports
  ports()->addPort("outPort", _outPort).doc("Receiver output port");
  if(_verbose)
    std::cout << "Mops_Broker constructed !" <<std::endl;
}

bool Mops_Broker::configureHook(){
  // Set component activitiy 
  if(setActivity(new RTT::Activity(ORO_SCHED_RT, 
				   _priority, 
				   0, 
				   _cpu, 
				   0, 
				   getName())) == false){
    if(_verbose)
      std::cout << "Unable to set activity!" << std::endl;
    return false;
  }
  if(_verbose)
    std::cout << "Mops_Broker configured !" <<std::endl;
  return true;
}

bool Mops_Broker::startHook(){
  StartMOPSBrokerNonBlocking();

  if(_verbose)
    std::cout << "Mops_Broker started !" <<std::endl;
  return true;
}

void Mops_Broker::updateHook(){
  if(_verbose)
    std::cout << "Mops_Broker executes updateHook !" <<std::endl;
}

void Mops_Broker::stopHook() {
  if(_verbose)
    std::cout << "Mops_Broker executes stopping !" <<std::endl;
}

void Mops_Broker::cleanupHook() {
  if(_verbose)
    std::cout << "Mops_Broker cleaning up !" <<std::endl;
}

/*
 * Using this macro, only one component may live
 * in one library *and* you may *not* link this library
 * with another component library. Use
 * ORO_CREATE_COMPONENT_TYPE()
 * ORO_LIST_COMPONENT_TYPE(Mops_Broker)
 * In case you want to link with another library that
 * already contains components.
 *
 * If you have put your component class
 * in a namespace, don't forget to add it here too:
 */
ORO_CREATE_COMPONENT(Mops_Broker)
