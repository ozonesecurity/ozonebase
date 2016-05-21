#ifndef LIBGEN_EXEC_H
#define LIBGEN_EXEC_H

#include <map>

typedef bool (*SvrInitHandler)();

typedef std::multimap<int,SvrInitHandler> SvrInitHandlersByPri;

bool addSvrInitFunc( SvrInitHandler func, int priority=10 );
bool svrInit();

bool addSvrTermFunc( SvrInitHandler func, int priority=10 );
bool svrTerm();

typedef void (*SvrSigHandler)( int );

typedef std::multimap<int,SvrSigHandler> SvrSigHandlersByPri;
typedef std::multimap<int,SvrSigHandlersByPri *> SvrSigHandlersBySig;

bool addSvrSigHandlerFunc( int signal, SvrSigHandler func, int priority=10 );

class SvrExecRegistration
{
public:
    SvrExecRegistration();
};

#endif // LIBGEN_EXEC_H
