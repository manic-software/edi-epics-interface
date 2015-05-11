#ifndef EDI_PV
#define EDI_PV

//epics crap
#include <casdef.h>
#include <gddAppFuncTable.h>

#include <QString>

class Server;

//process variable class
class PV : public casPV 
{
  friend class Server;

public:
  PV(Server*, QString, PV* =NULL);
  ~PV(void);

private:
  Server* m_server;
  QString m_name;
  QString m_units;
  gdd* m_value;
  int m_precision;
  aitBool m_interest;
  static int currentOps;
  PV* m_parent;
  PV* m_desc;
  PV* m_unit;
  PV* m_high;
  PV* m_hihi;
  PV* m_low;
  PV* m_lolo;

public:
  //virtual epics functions
  const char* getName(void) const;
  caStatus interestRegister();
  void interestDelete();
  caStatus beginTransaction();
  void endTransaction();
  caStatus read(const casCtx&, gdd&);
  caStatus write(const casCtx&, const gdd&);
  void destroy(void);
  aitEnum bestExternalType() const;
  //epics pv parameter accessors
  gddAppFuncTableStatus readPrecision(gdd&);
  gddAppFuncTableStatus readHighAlarm(gdd&);
  gddAppFuncTableStatus readHighWarn(gdd&);
  gddAppFuncTableStatus readLowWarn(gdd&);
  gddAppFuncTableStatus readLowAlarm(gdd&);
  gddAppFuncTableStatus readValue(gdd&);
  gddAppFuncTableStatus readUnits(gdd&);

private:
  //regular functions
  void set_alarms(void);
  
public:
  //more regular functions
  QString get_name(void) const;
  void set_value(double);
  void set_value(int);
  void set_value(QString);
  void set_precision(int);
  void set_highalarm(double);
  void set_highwarn(double);
  void set_lowalarm(double);
  void set_lowwarn(double);
  void set_units(QString);
  void set_description(QString);
};


#endif //ifndef EDI_PV
