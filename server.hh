#ifndef EDI_SERVER
#define EDI_SERVER

#include <QTcpSocket>
#include <QTcpServer>
#include <QThread>
#include <QMutex>

#include <map>
using std::map;

#include "pv.hh"

#include <vector>
using std::vector;

class EPICSLoop : public QThread
{
public:
  void run(void);
  void stop(void);

private:
  bool m_run;
};

class Server : public QTcpServer, public caServer
{
Q_OBJECT

public:
  Server(void);
  ~Server(void);

private:
  static gddAppFuncTable<PV> funcTable;
  vector<PV*> m_pvs;
  EPICSLoop m_thread;
  QMutex mutex;

private:
  static void initFuncTable(void);
  PV* find_pv(QString);
  PV* create_pv(QString);

public:
  static gddAppFuncTableStatus read(PV&, gdd&);
  pvAttachReturn pvAttach(const casCtx&, const char*);
  pvExistReturn pvExistTest(const casCtx&, const char*);

private slots:
  void cb_connection(void);
  void readClient(void);

};

#endif //ifndef EDI_SERVER
