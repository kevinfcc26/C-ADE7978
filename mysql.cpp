#include <iostream>
#include <mysql.h>
#include <stdio.h>
#include <string.h>

using namespace std;

#define SERVER "us-cdbr-east-05.cleardb.net"
#define USER "b62ba68668ea1c"
#define PASSWORD "4d0579fe"
#define DATABASE "heroku_851e4397b87123b"

int main(){
    //  string a="IAWV", pat1="INSERT INTO Burst ", pat2="VALUES";
    //  int b =1;
    //  //char patronnombre[] = "INSERT INTO Burst (\'%s\')";
    //  char patron2[]=" VALUES(\'%s\')";
    //  std::string con =""+pat1+'('+a+')'+' '+pat2+' '+'('+std::to_string(b)+')'+";";
    // //INSERT INTO Burst (IAWV) VALUES ('1');
    //  cout<<con;
	// char consulta[con.size() + 1];
	// strcpy(consulta, con.c_str());
    //  cout<<consulta;
    MYSQL *connect;
    connect=mysql_init(NULL);
    if (!connect){
        cout<<"MySQL Initialization failed";
        return 1;
    }
    connect=mysql_real_connect(connect, SERVER, USER, PASSWORD , DATABASE ,0,NULL,0);
    if (connect){
        cout<<"connection Succeeded\n";
    }
    else{
        cout<<"connection failed\n";
    }

    MYSQL_RES *res_set;
    MYSQL_ROW row;
    mysql_query (connect,"INSERT INTO `heroku_851e4397b87123b`.`test` (name) VALUES ('kevin');");
    unsigned int i =0;
    res_set = mysql_store_result(connect);
    unsigned int numrows = mysql_num_rows(res_set);

    cout << endl;
    cout <<"\t --------------------------------------------------------------------- \t"<< endl;

    while (((row= mysql_fetch_row(res_set)) !=NULL ))
    { //cout<<" %s\n",row[i] !=NULL?row[i] : "NULL";

    cout <<"\t | \t" << row[i] << "\t | \t"<<  row[i+1] << "\t | \t"<< row[i+2] << "\t | \t" << endl;
    cout <<"\t --------------------------------------------------------------------- \t"<< endl;
    }

    mysql_close (connect);
    return 0;
     
    //  mysql_query(connect, consulta);
     // Cerrar la conexión
    //  mysql_close(connect);     
}