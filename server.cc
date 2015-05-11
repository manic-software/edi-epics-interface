#include "server.hh"

#include <QRegExp>
#include <QTimer>
#include <QSqlError>
#include <QTextStream>
#include <QVariant>
#include <QProcess>

#include <iostream>
using std::cout;
using std::cerr;
using std::endl;

#include <sys/vfs.h>

#define DEFPORT 4875


#include <gdd.h>
#include <gddApps.h>
#include <epicsTime.h>
#include <fdManager.h>

#include <cadef.h>

gddAppFuncTable<PV> Server::funcTable;

Server::
Server(void):
  QTcpServer(),
  caServer()
{
  int port=DEFPORT;
  if (getenv("EDI_PORT")) port=atoi(getenv("EDI_PORT"));
  if (port<1025 || port>65535)
    port=DEFPORT;
  listen(QHostAddress::Any, port);
  QTcpServer::connect( this, SIGNAL(newConnection()), this, SLOT(cb_connection()) );

  initFuncTable();
  
  m_thread.start();

}
Server::
~Server(void)
{
  //delete all pvs
  //  cout << "server deleted\n";
  mutex.lock();
  for (int i=0; i<m_pvs.size(); i++)
    delete m_pvs[i];
  m_pvs.resize(0);
  mutex.unlock();
}

void Server::
initFuncTable(void)
{
  funcTable.installReadFunc("precision", &PV::readPrecision);
  funcTable.installReadFunc("alarmHigh", &PV::readHighAlarm);
  funcTable.installReadFunc("alarmHighWarning", &PV::readHighWarn);
  funcTable.installReadFunc("alarmLowWarning", &PV::readLowWarn);
  funcTable.installReadFunc("alarmLow", &PV::readLowAlarm);
  funcTable.installReadFunc("value", &PV::readValue);
  funcTable.installReadFunc("graphicHigh", &PV::readHighWarn);
  funcTable.installReadFunc("graphicLow", &PV::readLowWarn);
  funcTable.installReadFunc("controlHigh", &PV::readHighAlarm);
  funcTable.installReadFunc("controlLow", &PV::readLowAlarm);
  funcTable.installReadFunc("units", &PV::readUnits);
}

PV* Server::
find_pv(QString name)
{
  PV* ret=NULL;
  mutex.lock();
  for (int i=0; i<m_pvs.size(); i++)
    {
      if (m_pvs[i]->get_name()==name || m_pvs[i]->get_name()+".VAL"==name)
	ret=m_pvs[i];
      else if (m_pvs[i]->m_desc->get_name()==name)
	ret=m_pvs[i]->m_desc;
      else if (m_pvs[i]->m_high->get_name()==name)
	ret=m_pvs[i]->m_high;
      else if (m_pvs[i]->m_hihi->get_name()==name)
	ret=m_pvs[i]->m_hihi;
      else if (m_pvs[i]->m_low->get_name()==name)
	ret=m_pvs[i]->m_low;
      else if (m_pvs[i]->m_lolo->get_name()==name)
	ret=m_pvs[i]->m_lolo;
      else if (m_pvs[i]->m_unit->get_name()==name)
	ret=m_pvs[i]->m_unit;
    }
  mutex.unlock();
  return ret;
}
PV* Server::
create_pv(QString name)
{
  m_pvs.push_back(new PV(this, name));
  return m_pvs.back();
}
gddAppFuncTableStatus Server::
read(PV &pv, gdd &value)
{
  return Server::funcTable.read(pv, value);
}

pvAttachReturn Server::
pvAttach(const casCtx&, const char* cname)
{
  QString name(cname);
  //  cout << "pv attachment request for " << name << endl;
  mutex.lock();
  PV* ret=NULL;
  for (int i=0; i<m_pvs.size(); i++)
    {
      if (m_pvs[i]->get_name()==name || m_pvs[i]->get_name()+".VAL"==name)
	ret=m_pvs[i];
      else if (m_pvs[i]->m_desc->get_name()==name)
	ret=m_pvs[i]->m_desc;
      else if (m_pvs[i]->m_high->get_name()==name)
	ret=m_pvs[i]->m_high;
      else if (m_pvs[i]->m_hihi->get_name()==name)
	ret=m_pvs[i]->m_hihi;
      else if (m_pvs[i]->m_low->get_name()==name)
	ret=m_pvs[i]->m_low;
      else if (m_pvs[i]->m_lolo->get_name()==name)
	ret=m_pvs[i]->m_lolo;
      else if (m_pvs[i]->m_unit->get_name()==name)
	ret=m_pvs[i]->m_unit;
    }
  mutex.unlock();
  if (!ret)
    return S_casApp_pvNotFound;
  else
    {
//       cout << "attached " << cname << endl;
	pvAttachReturn ret1(*(ret));
	return ret1;
    }
}

pvExistReturn Server::
pvExistTest(const casCtx&, const char* cname)
{
  QString name(cname);
  mutex.lock();
  PV* ret=NULL;
  for (int i=0; i<m_pvs.size(); i++)
    {
      if (m_pvs[i]->get_name()==name || m_pvs[i]->get_name()+".VAL"==name)
	ret=m_pvs[i];
      else if (m_pvs[i]->m_desc->get_name()==name)
	ret=m_pvs[i]->m_desc;
      else if (m_pvs[i]->m_high->get_name()==name)
	ret=m_pvs[i]->m_high;
      else if (m_pvs[i]->m_hihi->get_name()==name)
	ret=m_pvs[i]->m_hihi;
      else if (m_pvs[i]->m_low->get_name()==name)
	ret=m_pvs[i]->m_low;
      else if (m_pvs[i]->m_lolo->get_name()==name)
	ret=m_pvs[i]->m_lolo;
      else if (m_pvs[i]->m_unit->get_name()==name)
	ret=m_pvs[i]->m_unit;
    }
  mutex.unlock();
//   if (ret)
//     cout << "exists " << cname << endl;
  if (ret)
    return pverExistsHere;
  else
    return pverDoesNotExistHere;
}

void EPICSLoop::
run(void)
{
  m_run=true;
  while (m_run)
    fileDescriptorManager.process(1.0);
}
void EPICSLoop::
stop(void)
{
  m_run=false;
}

void Server::
cb_connection(void)
{
  QTcpSocket* s = nextPendingConnection();
  QTcpServer::connect( s, SIGNAL(readyRead()), this, SLOT(readClient()) );
  QTcpServer::connect( s, SIGNAL(disconnected()), s, SLOT(deleteLater()) );
}

const char* alarmStatusString(short s)
{
  if (s==NO_ALARM)
    return "SAFE";
  else if (s==HIGH_ALARM)
    return "HIGH";
  else if (s==HIHI_ALARM)
    return "HIHI";
  else if (s==LOW_ALARM)
    return "LOW";
  else if (s==LOLO_ALARM)
    return "LOLO";
  else
    return "UNKNOWN";
}
const char* alarmSeverityString(short s)
{
  if (s==NO_ALARM)
    return "SAFE";
  else if (s==MINOR_ALARM)
    return "MINOR";
  else if (s==MAJOR_ALARM)
    return "MAJOR";
  else
    return "UNKNOWN";
}

void Server::
readClient(void)
{
  QTcpSocket* socket=(QTcpSocket*)sender();
  QTextStream os(socket);
  while ( socket->canReadLine() ) {
    QString line=socket->readLine();
    QStringList tokens = line.split( QRegExp("\\s(?=[^\"]*(\"[^\"]*\"[^\"]*)*$)") );
    tokens.replaceInStrings("\"", "");
    //cout << tokens.join("|") << endl;

    if (tokens.size()<1)
      {
	cerr << "invalid command  " << qPrintable(tokens.join(" ")) << endl;
	return;
      }
    if (tokens[0].toLower()=="list")
      {
	mutex.lock();
	for (int i=0; i<m_pvs.size(); i++)
	  {
	    os << m_pvs[i]->get_name() << endl;
	  }
	mutex.unlock();
	os << endl;
      }
    else if (tokens[0].toLower()=="create")
      {
	if (tokens.size()<2)
	  cerr << "invalid command " << qPrintable(tokens.join(" ")) << endl;
	else
	  {
	    PV* pv=find_pv(tokens[1]);
	    if (pv)
	      os << "exists \"" << tokens[1] << "\"" << endl;
	    else
	      {
		create_pv(tokens[1]);
		os << "created \"" << tokens[1] << "\"" << endl;
	      }
	  }
      }
    else if (tokens[0].toLower()=="value")
      {
	if (tokens.size()<3)
	  cerr << "invalid command " << qPrintable(tokens.join(" ")) << endl;
	else
	  {
	    PV* pv=find_pv(tokens[1]);
	    if (!pv)
	      os << "ERROR_NOEXIST" << endl;
	    else
	      {
		if (tokens.size()>3 && tokens[3].toLower()=="integer")
		  pv->set_value(tokens[2].toInt());
		else if (tokens.size()>3 && tokens[3].toLower()=="string")
		  pv->set_value(tokens[2]);
		else
		  pv->set_value(tokens[2].toDouble());
		os << "OKAY" << endl;
	      }
	  }
      }
    else if (tokens[0].toLower()=="describe")
      {
	if (tokens.size()<3)
	  cerr << "invalid command " << qPrintable(tokens.join(" ")) << endl;
	else
	  {
	    PV* pv=find_pv(tokens[1]+".DESC");
	    if (!pv)
	      os << "ERROR_NOEXIST" << endl;
	    else
	      {
		pv->set_value(tokens[2]);
		os << "OKAY" << endl;
	      }
	  }

      }
    else if (tokens[0].toLower()=="precision")
      {
	if (tokens.size()<3)
	  cerr << "invalid command " << qPrintable(tokens.join(" ")) << endl;
	else
	  {
	    PV* pv=find_pv(tokens[1]);
	    if (!pv)
	      os << "ERROR_NOEXIST" << endl;
	    else
	      {
		pv->set_precision(tokens[2].toInt());
		os << "OKAY" << endl;
	      }
	  }
      }
    else if (tokens[0].toLower()=="units")
      {
	if (tokens.size()<3)
	  cerr << "invalid command " << qPrintable(tokens.join(" ")) << endl;
	else
	  {
	    PV* pv=find_pv(tokens[1]);
	    if (!pv)
	      os << "ERROR_NOEXIST" << endl;
	    else
	      {
		pv->set_units(tokens[2]);
		os << "OKAY" << endl;
	      }
	  }
      }
    else if (tokens[0].toLower()=="lowalarm")
      {
	if (tokens.size()<3)
	  cerr << "invalid command " << qPrintable(tokens.join(" ")) << endl;
	else
	  {
	    PV* pv=find_pv(tokens[1]);
	    if (!pv)
	      os << "ERROR_NOEXIST" << endl;
	    else
	      {
		pv->set_lowalarm(tokens[2].toDouble());
		os << "OKAY" << endl;
	      }
	  }
      }
    else if (tokens[0].toLower()=="lowwarn")
      {
	if (tokens.size()<3)
	  cerr << "invalid command " << qPrintable(tokens.join(" ")) << endl;
	else
	  {
	    PV* pv=find_pv(tokens[1]);
	    if (!pv)
	      os << "ERROR_NOEXIST" << endl;
	    else
	      {
		pv->set_lowwarn(tokens[2].toDouble());
		os << "OKAY" << endl;
	      }
	  }
      }
    else if (tokens[0].toLower()=="highalarm")
      {
	if (tokens.size()<3)
	  cerr << "invalid command " << qPrintable(tokens.join(" ")) << endl;
	else
	  {
	    PV* pv=find_pv(tokens[1]);
	    if (!pv)
	      os << "ERROR_NOEXIST" << endl;
	    else
	      {
		pv->set_highalarm(tokens[2].toDouble());
		os << "OKAY" << endl;
	      }
	  }
      }
    else if (tokens[0].toLower()=="highwarn")
      {
	if (tokens.size()<3)
	  cerr << "invalid command " << qPrintable(tokens.join(" ")) << endl;
	else
	  {
	    PV* pv=find_pv(tokens[1]);
	    if (!pv)
	      os << "ERROR_NOEXIST" << endl;
	    else
	      {
		pv->set_highwarn(tokens[2].toDouble());
		os << "OKAY" << endl;
	      }
	  }
      }
    else if (tokens[0].toLower()=="write")
      {
	if (tokens.size()<3)
	  cerr <<  "invalid command " << qPrintable(tokens.join(" ")) << endl;
	else
	  {
	    chid chan;
	    int status=ca_create_channel ( qPrintable(tokens[1]), 0, 0, 0, & chan );
	    SEVCHK ( status, "ca_create_channel()" );
	    status = ca_pend_io ( 15.0 );
	    if ( status != ECA_NORMAL ) {
	      cerr << "PV " << qPrintable(tokens[1]) << " not found\n";
	      os << "ERROR_NOTFOUND" << endl;
	    }
	    else {
	      cout << "connected to PV \"" << qPrintable(tokens[1]) << "\" on " << ca_host_name(chan) << "\n";
		if (tokens.size()>3 && tokens[3].toLower()=="integer")
		  {
		    epicsInt32 data=tokens[2].toInt();
		    status=ca_put(DBR_LONG, chan, &data);
		  }
		else if (tokens.size()>3 && tokens[3].toLower()=="string")
		  {
		    status=ca_bput(chan, qPrintable(tokens[2]));
		  }
		else
		  {
		    epicsFloat64 data=tokens[2].toDouble();
		    status=ca_put(DBR_DOUBLE, chan, &data);
		  }
		if (status!=ECA_NORMAL) {
		  cerr << "Put failed\n";
		  os << "ERROR_WRITEFAIL" << endl;
		} 
		status = ca_pend_io ( 15.0 );
		if (status!=ECA_NORMAL) {
		  cerr << "Put failed\n";
		  os << "ERROR_WRITEFAIL" << endl;
		} else {
		  os << "OKAY: " << ca_host_name(chan) << endl;
		}
	      ca_clear_channel ( chan );
	      ca_task_exit ();
		
	    }
	      
	  }
      }
    else if (tokens[0].toLower()=="read")
      {
	if (tokens.size()<2)
	  cerr << "invalid command " << qPrintable(tokens.join(" ")) << endl;
	else
	  {
	    chid chan;
	    int status=ca_create_channel ( qPrintable(tokens[1]), 0, 0, 0, & chan );
	    SEVCHK ( status, "ca_create_channel()" );
	    status = ca_pend_io ( 15.0 );
	    if ( status != ECA_NORMAL ) {
	      cerr << "PV " << qPrintable(tokens[1]) << " not found\n";
	      os << "ERROR_NOTFOUND" << endl;
	    }
	    else {
	      cout << "connected to PV \"" << qPrintable(tokens[1]) << "\" on " << ca_host_name(chan) << "\n";
	      os << "OKAY: " << ca_host_name(chan) << endl;
	      chtype typ=ca_field_type(chan);
	      //shift to CTRL type to gather maximum information
	      if (typ!=DBR_STRING)
		{
		  if (dbr_type_is_plain(typ))
		    typ+=(DBR_CTRL_DOUBLE-DBR_DOUBLE);
		  else if (dbr_type_is_STS(typ))
		    typ+=(DBR_CTRL_DOUBLE-DBR_STS_DOUBLE);
		  else if (dbr_type_is_GR(typ))
		    typ+=(DBR_CTRL_DOUBLE-DBR_GR_DOUBLE);
		  if (typ==DBR_CTRL_FLOAT) typ=DBR_CTRL_DOUBLE;
		  if (typ==DBR_CTRL_INT) typ=DBR_CTRL_LONG;
		}
	      //int8, char and enum currently not handled
	      unsigned elementCount = ca_element_count ( chan );
	      unsigned nBytes = dbr_size_n ( typ, elementCount );
	      char* mem=new char[nBytes];
	      if ( ! mem ) {
		cerr << "PV memory allocation failed " << qPrintable(tokens[1]) << endl;
		os << "ERROR_MALLOC" << endl;
	      } 
	      else {

		int status=ca_array_get(typ, elementCount, chan, mem);
		SEVCHK(status, "ca_array_get()");
		if (status!=ECA_NORMAL)
		  {
		    cerr << "PV get failed " << qPrintable(tokens[1]) << endl;
		    os << "ERROR_GET" << endl;
		  }
		else
		  {
		    int status=ca_pend_io(15.0);
		    if ( status != ECA_NORMAL ) {
		      cerr << "PV " << qPrintable(tokens[1]) << " get timed out\n";
		      os << "ERROR_TIMEOUT" << endl;
		    }
		    else
		      {
			//			char timestr[64];
			switch (typ)
			  {
			  case DBR_STRING:
			    {
			      dbr_string_t* pTD =(dbr_string_t*)mem;
			      os << "Name: \"" << qPrintable(tokens[1]) << "\"" << endl;
			      os << "Type: String" << endl;
			      os << "Value: \"" << (char*)pTD << "\"" << endl;
			      // 		      epicsTimeToStrftime(timestr, sizeof(timestr),
			      // 					  "%a %b %d %Y %H:%M:%S.%f", 
			      // 					  &pTD->stamp );
			      //			      os << "Modified: " << timestr << endl;
			      os << endl;

			      break;
			    }
			  case DBR_CTRL_LONG:
			    {
			      struct dbr_ctrl_long* pTD =(struct dbr_ctrl_long*)mem;
			      os << "Name: \"" << qPrintable(tokens[1]) << "\"" << endl;
			      os << "Type: Long/Int" << endl;
			      os << "Value: " << pTD->value << endl;
			      //			      os << "Modified: " << timestr << endl;
			      os << "Status: " << alarmStatusString(pTD->status) << endl;
			      os << "Severity: " << alarmSeverityString(pTD->severity) << endl;
			      os << "Units: \"" << pTD->units << "\"" << endl;
			      os << "UpperAlarmLimit: " << pTD->upper_alarm_limit << endl;
			      os << "UpperWarningLimit: " << pTD->upper_warning_limit << endl;
			      os << "LowerWarningLimit: " << pTD->lower_warning_limit << endl;
			      os << "LowerAlarmLimit: " << pTD->lower_alarm_limit << endl;
			      os << endl;
			      break;
			    }
			  case DBR_CTRL_DOUBLE:
			    {
			      struct dbr_ctrl_double* pTD =(struct dbr_ctrl_double*)mem;
			      os << "Name: \"" << qPrintable(tokens[1]) << "\"" << endl;
			      os << "Type: Double/Float" << endl;
			      os << "Value: " << pTD->value << endl;
			      //			      os << "Modified: " << timestr << endl;
			      os << "Status: " << alarmStatusString(pTD->status) << endl;
			      os << "Severity: " << alarmSeverityString(pTD->severity) << endl;
			      os << "Precision: " << pTD->precision << endl;
			      os << "Units: \"" << pTD->units << "\"" << endl;
			      os << "UpperAlarmLimit: " << pTD->upper_alarm_limit << endl;
			      os << "UpperWarningLimit: " << pTD->upper_warning_limit << endl;
			      os << "LowerWarningLimit: " << pTD->lower_warning_limit << endl;
			      os << "LowerAlarmLimit: " << pTD->lower_alarm_limit << endl;
			      os << endl;
			      // 		      cout << "UpperDispLimit: " << pTD->upper_disp_limit << endl;
			      // 		      cout << "LowerDispLimit: " << pTD->lower_disp_limit << endl;
			      // 		      cout << "UpperCtrlLimit: " << pTD->upper_ctrl_limit << endl;
			      // 		      cout << "LowerCtrlLimit: " << pTD->lower_ctrl_limit << endl;
			      break;
			    }

			  default:
			    cerr << "unhandled epics data type " << typ << endl;
			  }
		
			delete[] mem;
		
			//		epicsTimeToStrftime ( timeString, sizeof ( timeString ),
			//				      "%a %b %d %Y %H:%M:%S.%f", & pTD->stamp );
		      }
		  }
	      }
	      ca_clear_channel ( chan );
	      ca_task_exit ();

	    }
	  }
      }
  }
}
