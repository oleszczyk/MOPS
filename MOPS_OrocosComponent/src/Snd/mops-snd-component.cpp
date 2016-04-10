#include "mops-snd-component.hpp"
#include <rtt/Component.hpp>
#include <iostream>
#include <string>

static const int         defPriority = 10;
static const unsigned    defCpu      = 0;
static const unsigned    defTimeout  = 500; // [ms]
static std::string       defTopicName= "";

Mops_Snd::Mops_Snd(std::string const& name) :
  TaskContext(name, PreOperational),
  _verbose(false),
  _priority(defPriority),
  _cpu(defCpu),
  _timeout(defTimeout),
  _topicName(defTopicName)
{
  // Add attributes 
  addAttribute("verbose",   _verbose);
  addAttribute("priority",  _priority);
  addAttribute("cpu",       _cpu);
  addAttribute("timeout",   _timeout);
  addAttribute("topicName", _topicName);

  // Add input ports
  ports()->addEventPort("inPort", _inPort).doc("Sender input port");
  if(_verbose)
    std::cout << "Mops_Snd constructed !" <<std::endl;
}

bool Mops_Snd::configureHook(){
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
    std::cout << "Mops_Snd configured !" <<std::endl;
  return true;
}

bool Mops_Snd::startHook(){
  if(_verbose)
    std::cout << "Mops_Snd started !" <<std::endl;
  return true;
}

void Mops_Snd::updateHook(){
  MyData msg;

  if( _inPort.read(msg)==RTT::NewData && _topicName.compare(defTopicName) )
  {
    publishMOPS((char*)_topicName.c_str(), (char*)msg.data);
    getActivity()->trigger();
    if(_verbose)
      std::cout << "Mops_Snd executes updateHook !" <<std::endl;
  }
  else{
    usleep(1000 * _timeout);
    getActivity()->trigger();  
  }
}

void Mops_Snd::stopHook() {
  if(_verbose)
    std::cout << "Mops_Snd executes stopping !" <<std::endl;
}

void Mops_Snd::cleanupHook() {
  if(_verbose)
    std::cout << "Mops_Snd cleaning up !" <<std::endl;
}

/*
 * Using this macro, only one component may live
 * in one library *and* you may *not* link this library
 * with another component library. Use
 * ORO_CREATE_COMPONENT_TYPE()
 * ORO_LIST_COMPONENT_TYPE(Mops_Snd)
 * In case you want to link with another library that
 * already contains components.
 *
 * If you have put your component class
 * in a namespace, don't forget to add it here too:
 */
ORO_CREATE_COMPONENT(Mops_Snd)
