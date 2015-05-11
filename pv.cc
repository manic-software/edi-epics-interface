#include "pv.hh"

#include <gdd.h>
#include <gddApps.h>
#include <epicsTime.h>
#include <fdManager.h>

#include <cadef.h>

#include <iostream>
using std::cout;
using std::cerr;
using std::endl;

#include "server.hh"

//#include <limits>
//using std::numeric_limits;

int PV::currentOps;

PV::
PV(Server* svr, QString name, PV* parent):
  casPV(*dynamic_cast<caServer*>(svr)),
  m_server(svr),
  m_name(name),
  m_precision(4),
  m_parent(parent),
  m_desc(NULL),
  m_unit(NULL),
  m_high(NULL),
  m_hihi(NULL),
  m_low(NULL),
  m_lolo(NULL)
{
  if (m_parent)
    {
      //i am a child
    }
  else
    {
      //i am the parent
      //create appropriate children
      m_desc=new PV(m_server, name+".DESC", this);
      m_desc->m_value->put(qPrintable("Description of "+name));
      m_unit=new PV(m_server, name+".EGU", this);
      m_unit->m_value->put("");
      m_high=new PV(m_server, name+".HIGH", this);
      m_high->m_value->put(9e5);
      m_hihi=new PV(m_server, name+".HIHI", this);
      m_hihi->m_value->put(1e6);
      m_low=new PV(m_server, name+".LOW", this);
      m_low->m_value->put(-9e5);
      m_lolo=new PV(m_server, name+".LOLO", this);
      m_lolo->m_value->put(-1e6);
    }

  m_value=new gddScalar(gddAppType_value);
  m_value->put((aitFloat64)0.0);
  // Set the timespec structure to the current time stamp the gdd.
  aitTimeStamp t=epicsTime::getCurrent();
  m_value->setTimeStamp(&t);

  set_alarms();
  m_value->reference();
}
PV::
~PV(void)
{
  //  cout << "pv " << qPrintable(m_name) << " deleted\n";
}

const char* PV::getName(void) const
{
  return qPrintable(m_name);
}
caStatus PV::interestRegister(void)
  {
    m_interest = aitTrue;
    return S_casApp_success;
  }
void PV::interestDelete(void)
{
  m_interest=aitFalse;
}

caStatus PV::beginTransaction()
{
  // Trivial implementation that informs the user of the number of
  // current IO operations in progress for the server tool. currentOps
  //  is a static member.
  currentOps++;
  return S_casApp_success;
}

void PV::endTransaction()
{
  currentOps--;
}

caStatus PV::read(const casCtx&, gdd &prototype)
{
  // Calls myServer::read() which calls the appropriate function
  // from the application table.
  //   aitFloat64 v;
  //   m_value->get(&v);
  //   cout << "**returning value " << (double)v << " from read\n";
//   m_value->put(v+1);
  return Server::read(*this, prototype);
}


gddAppFuncTableStatus PV::readPrecision(gdd &value)
{
  value.putConvert(m_precision);
  return S_casApp_success;
}

gddAppFuncTableStatus PV::readHighAlarm(gdd &value)
{
  if (m_parent)
    //i am a child with no alarms
    value.put(0.0);
  else
    {
      aitFloat64 v;
      m_hihi->m_value->get(v);
      value.putConvert(v);
    }
  return S_casApp_success;
}

gddAppFuncTableStatus PV::readHighWarn(gdd &value)
{
  if (m_parent)
    //i am a child with no alarms
    value.put(0.0);
  else
    {
      aitFloat64 v;
      m_high->m_value->get(v);
      value.putConvert(v);
    }
  //value.putConvert(m_highwarn);
  return S_casApp_success;
}

gddAppFuncTableStatus PV::readLowWarn(gdd &value)
{
  if (m_parent)
    //i am a child with no alarms
    value.put(0.0);
  else
    {
      aitFloat64 v;
      m_low->m_value->get(v);
      value.putConvert(v);
    }
  //value.putConvert(m_lowwarn);
  return S_casApp_success;
}

gddAppFuncTableStatus PV::readLowAlarm(gdd &value)
{
  if (m_parent)
    //i am a child with no alarms
    value.put(0.0);
  else
    {
      aitFloat64 v;
      m_lolo->m_value->get(v);
      value.putConvert(v);
    }
  //value.putConvert(m_lowalarm);
  return S_casApp_success;
}

gddAppFuncTableStatus PV::readValue(gdd &value)
{
  // If pvAttr::pValue exists, then use the gdd::get() function to 
  // assign the current value of pValue to currentVal; then use the
  // gdd::putConvert() to write the value into value.
  if(!m_value)
    return S_casApp_undefined;
  else {
    if (gddApplicationTypeTable::app_table.smartCopy(&value, m_value))
      return S_cas_noConvert;
    else
      return S_casApp_success;
  }
}

gddAppFuncTableStatus PV::readUnits(gdd &value)
{
  value.put(qPrintable(m_units));
  return S_casApp_success;
}

// bestExternalType() is a virtual function that can redefined to
// return the best type with which to access the PV. Called by the
// server library to respond to client request for the best type.
aitEnum PV::bestExternalType() const
{
  if(!m_value)
    return aitEnumInvalid;
  else
    return m_value->primitiveType();
}
caStatus PV::write(const casCtx&, const gdd &value)
{
  //check the name of this process variable to see if it is a secondary
  // Doesn't support writing to arrays or container objects
  // (gddAtomic or gddContainer).
  if(!(value.isScalar()))
    {

      return S_casApp_noSupport;
    }

  // Set the timespec structure to the current time stamp the gdd.
  aitTimeStamp t=epicsTime::getCurrent();
  m_value->setTimeStamp(&t);

  // Get the new value and set the severity and status according
  // to its value.
  gddApplicationTypeTable::app_table.smartCopy(m_value, &value);
  set_alarms();
  if(m_interest == aitTrue){
    casEventMask select(m_server->valueEventMask() |
			m_server->alarmEventMask());
    postEvent(select, *m_value);
  }
  return S_casApp_success;
}

void PV::
destroy(void)
{
  //  cout << "avoid deleting PV\n";
}

void PV::
set_alarms(void)
{
  
  if (m_parent)
    {
      //i am a child and I shall indicate safe status always
      m_value->setStat(0);
      m_value->setSevr(0);
      return;
    }
  switch (bestExternalType())
    {
    case aitEnumFloat64:
    case aitEnumFloat32:
      {
	aitFloat64 newVal;
	m_value->get(newVal);
	aitFloat64 high, hihi, low, lolo;
	m_high->m_value->get(high);
	m_low->m_value->get(low);
	m_hihi->m_value->get(hihi);
	m_lolo->m_value->get(lolo);
	
	if (newVal > hihi){
	  m_value->setStat(epicsAlarmHiHi);
	  m_value->setSevr(epicsSevMajor);
	}
	else if (newVal >= high){
	  m_value->setStat(epicsAlarmHigh);
	  m_value->setSevr(epicsSevMinor);
	}
	else if (newVal <lolo ){
	  m_value->setStat(epicsAlarmLoLo);
	  m_value->setSevr(epicsSevMajor);
	}
	else if (newVal <= low){
	  m_value->setStat(epicsAlarmLow);
	  m_value->setSevr(epicsSevMinor);
	}
	else 
	  {
	  m_value->setStat(0);
	  m_value->setSevr(0);
	  }
	break;
      }
    case aitEnumInt32:
    case aitEnumInt16:
    case aitEnumInt8:
      {
	aitInt32 newVal;
	m_value->get(newVal);
	aitInt32 high, hihi, low, lolo;
	m_high->m_value->get(high);
	m_low->m_value->get(low);
	m_hihi->m_value->get(hihi);
	m_lolo->m_value->get(lolo);	
	if (newVal > hihi){
	  m_value->setStat(epicsAlarmHiHi);
	  m_value->setSevr(epicsSevMajor);
	}
	else if (newVal >= high){
	  m_value->setStat(epicsAlarmHigh);
	  m_value->setSevr(epicsSevMinor);
	}
	else if (newVal <lolo ){
	  m_value->setStat(epicsAlarmLoLo);
	  m_value->setSevr(epicsSevMajor);
	}
	else if (newVal <= low){
	  m_value->setStat(epicsAlarmLow);
	  m_value->setSevr(epicsSevMinor);
	}
	else {
	  m_value->setStat(0);
	  m_value->setSevr(0);
	}
	break;
      }
    default:
      m_value->setStat(0);
      m_value->setSevr(0);
      break;
    };


}

QString PV::
get_name(void) const
{
  return m_name;
}
void PV::
set_value(double val)
{
  // Set the timespec structure to the current time stamp the gdd.
  aitTimeStamp t=epicsTime::getCurrent();
  m_value->setTimeStamp(&t);
  m_value->put((aitFloat64)val);
  set_alarms();
  if(m_interest == aitTrue){
    casEventMask select(m_server->valueEventMask() |
                        m_server->alarmEventMask());
    postEvent(select, *m_value);
  }

}
void PV::
set_value(int val)
{
  // Set the timespec structure to the current time stamp the gdd.
  aitTimeStamp t=epicsTime::getCurrent();
  m_value->setTimeStamp(&t);
  m_value->put((aitInt32)val);
  set_alarms();
  if(m_interest == aitTrue){
    casEventMask select(m_server->valueEventMask() |
                        m_server->alarmEventMask());
    postEvent(select, *m_value);
  }

}
void PV::
set_value(QString val)
{
  // Set the timespec structure to the current time stamp the gdd.
  aitTimeStamp t=epicsTime::getCurrent();
  m_value->setTimeStamp(&t);
  m_value->put(qPrintable(val));
  set_alarms();
  if(m_interest == aitTrue){
    casEventMask select(m_server->valueEventMask() |
                        m_server->alarmEventMask());
    postEvent(select, *m_value);
  }

}
void PV::
set_precision(int val)
{
  m_precision=val;
}
void PV::
set_highalarm(double val)
{
  if (m_parent)
    {
      //i am a child without my own alarm
      return;
    }
  switch (bestExternalType())
    {
    case aitEnumFloat64:
    case aitEnumFloat32:
      {
	m_hihi->m_value->put((aitFloat64)val);
	break;
      }
    case aitEnumInt32:
    case aitEnumInt16:
    case aitEnumInt8:
      {
	m_hihi->m_value->put((aitInt32)val);
	break;
      }
    default:
      break;
    };
  set_alarms();
}
void PV::
set_highwarn(double val)
{
  if (m_parent)
    {
      //i am a child without my own alarm
      return;
    }
  switch (bestExternalType())
    {
    case aitEnumFloat64:
    case aitEnumFloat32:
      {
	m_high->m_value->put((aitFloat64)val);
	break;
      }
    case aitEnumInt32:
    case aitEnumInt16:
    case aitEnumInt8:
      {
	m_high->m_value->put((aitInt32)val);
	break;
      }
    default:
      break;
    };
  set_alarms();
}
void PV::
set_lowalarm(double val)
{
  if (m_parent)
    {
      //i am a child without my own alarm
      return;
    }
  switch (bestExternalType())
    {
    case aitEnumFloat64:
    case aitEnumFloat32:
      {
	m_lolo->m_value->put((aitFloat64)val);
	break;
      }
    case aitEnumInt32:
    case aitEnumInt16:
    case aitEnumInt8:
      {
	m_lolo->m_value->put((aitInt32)val);
	break;
      }
    default:
      break;
    };
  set_alarms();
}
void PV::
set_lowwarn(double val)
{
  if (m_parent)
    {
      //i am a child without my own alarm
      return;
    }
  switch (bestExternalType())
    {
    case aitEnumFloat64:
    case aitEnumFloat32:
      {
	m_low->m_value->put((aitFloat64)val);
	break;
      }
    case aitEnumInt32:
    case aitEnumInt16:
    case aitEnumInt8:
      {
	m_low->m_value->put((aitInt32)val);
	break;
      }
    default:
      break;
    };
  set_alarms();
}

void PV::
set_units(QString val)
{
  m_units=val;
  if (!m_parent)
    m_unit->m_value->put(qPrintable(val));
}
void PV::
set_description(QString val)
{
  if (!m_parent)
    m_desc->m_value->put(qPrintable(val));
}
