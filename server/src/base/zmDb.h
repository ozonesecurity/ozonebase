#ifndef ZM_DB_H
#define ZM_DB_H

#include <mysql/mysql.h>

#include "zm.h"

extern MYSQL gDbConn;

void zmDbConnect();

#endif // ZM_DB_H
