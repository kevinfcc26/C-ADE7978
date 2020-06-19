// #include <iostream>
// #include <mysql.h>
// #include <stdio.h>
// #include <string.h>

// using namespace std;

// #define SERVER "us-cdbr-east-05.cleardb.net"
// #define USER "b62ba68668ea1c"
// #define PASSWORD "4d0579fe"
// #define DATABASE "heroku_851e4397b87123b"

// int main(){
//     //  string a="IAWV", pat1="INSERT INTO Burst ", pat2="VALUES";
//     //  int b =1;
//     //  //char patronnombre[] = "INSERT INTO Burst (\'%s\')";
//     //  char patron2[]=" VALUES(\'%s\')";
//     //  std::string con =""+pat1+'('+a+')'+' '+pat2+' '+'('+std::to_string(b)+')'+";";
//     // //INSERT INTO Burst (IAWV) VALUES ('1');
//     //  cout<<con;
// 	// char consulta[con.size() + 1];
// 	// strcpy(consulta, con.c_str());
//     //  cout<<consulta;
//      MYSQL *connect;
//      connect=mysql_init(NULL);
//      if (!connect){
//           cout<<"MySQL Initialization failed";
//           return 1;
//      }
//      connect=mysql_real_connect(connect, SERVER, USER, PASSWORD , DATABASE ,0,NULL,0);
//      if (connect){
//           cout<<"connection Succeeded\n";
//      }
//      else{
//           cout<<"connection failed\n";
//      }
     
//     //  mysql_query(connect, consulta);
//      // Cerrar la conexión
//      mysql_close(connect);     
// }

/* Standard C++ includes */
#include <stdlib.h>
#include <iostream>

/*
  Include directly the different
  headers from cppconn/ and mysql_driver.h + mysql_util.h
  (and mysql_connection.h). This will reduce your build time!
*/
#include "mysql_connection.h"

#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>

#define SERVER "us-cdbr-east-05.cleardb.net"
#define USER "b62ba68668ea1c"
#define PASSWORD "4d0579fe"
#define DATABASE "heroku_851e4397b87123b"

using namespace std;

int main(void)
{
cout << endl;
cout << "Running 'SELECT 'Hello World!' »
   AS _message'..." << endl;

try {
  sql::Driver *driver;
  sql::Connection *con;
  sql::Statement *stmt;
  sql::ResultSet *res;

  /* Create a connection */
  driver = get_driver_instance();
  con = driver->connect(SERVER, USER, PASSWORD, DATABASE);
  /* Connect to the MySQL test database */
  con->setSchema("test");

  stmt = con->createStatement();
  res = stmt->executeQuery("SELECT 'Hello World!' AS _message");
  while (res->next()) {
    cout << "\t... MySQL replies: ";
    /* Access column data by alias or column name */
    cout << res->getString("_message") << endl;
    cout << "\t... MySQL says it again: ";
    /* Access column data by numeric offset, 1 is the first column */
    cout << res->getString(1) << endl;
  }
  delete res;
  delete stmt;
  delete con;

} catch (sql::SQLException &e) {
  cout << "# ERR: SQLException in " << __FILE__;
  cout << "(" << __FUNCTION__ << ") on line " »
     << __LINE__ << endl;
  cout << "# ERR: " << e.what();
  cout << " (MySQL error code: " << e.getErrorCode();
  cout << ", SQLState: " << e.getSQLState() << " )" << endl;
}

cout << endl;

return EXIT_SUCCESS;
}