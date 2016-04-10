#include "mops-rec-component.hpp"
#include <rtt/Component.hpp>
#include <iostream>
#include <string>
#include <list>

static const int         defPriority = 10;
static const unsigned    defCpu      = 0;

Mops_Rec::Mops_Rec(std::string const& name) :
  TaskContext(name, PreOperational),
  _verbose(false),
  _priority(defPriority),
  _cpu(defCpu)
{
  // Add operations 
  addOperation("addSub", 
	       &Mops_Rec::_addSub, 
	       this, 
	       RTT::OwnThread).
    doc("Add subscribtion topic").
    arg("topic", "New topic to subscribe");
  // Add attributes 
  addAttribute("verbose",  _verbose);
  addAttribute("priority", _priority);
  addAttribute("cpu",      _cpu);

  // Add output ports
  ports()->addPort("outPort", _outPort).doc("Receiver output port");
  if(_verbose)
    std::cout << "Mops_Rec constructed !" <<std::endl;
}

bool Mops_Rec::configureHook(){
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
  
  connectToMOPS();
  if(_verbose)
    std::cout << "Mops_Rec configured !" <<std::endl;
  return true;
}

bool Mops_Rec::startHook(){
  if(_verbose)
    std::cout << "Mops_Rec started !" <<std::endl;
  return true;
}

void Mops_Rec::updateHook(){ 
  MyData msg;
  readMOPS((char*)msg.data, MAX_MESSAGE_LENGTH);

  _outPort.write(msg);
  getActivity()->trigger();
  if(_verbose)
    std::cout << "Mops_Rec executes updateHook !" <<std::endl;
}

void Mops_Rec::stopHook() {
  if(_verbose)
    std::cout << "Mops_Rec executes stopping !" <<std::endl;
}

void Mops_Rec::cleanupHook() {
  if(_verbose)
    std::cout << "Mops_Rec cleaning up !" <<std::endl;
}

void Mops_Rec::_addSub(std::string topic) {
  uint8_t Qos = 0;
  const char * str = topic.c_str(); 
  subscribeMOPS((char**)&str, &Qos, 1);
}

/*
 * Using this macro, only one component may live
 * in one library *and* you may *not* link this library
 * with another component library. Use
 * ORO_CREATE_COMPONENT_TYPE()
 * ORO_LIST_COMPONENT_TYPE(Mops_Rec)
 * In case you want to link with another library that
 * already contains components.
 *
 * If you have put your component class
 * in a namespace, don't forget to add it here too:
 */
ORO_CREATE_COMPONENT(Mops_Rec)
