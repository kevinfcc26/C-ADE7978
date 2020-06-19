# CFLAGS=-I/include
# LDFLAGS=-L/lib
# LDLIBS=-lbcm2835

# I2c: I2c.o
# 	g++ $(CFLAGS) $(LDFLAGSS) I2c.o $(LDLIBS) -o I2c
# I2c.o: I2c.cpp
# 	g++ -c -o I2c.o I2c.cpp

	
# .PONY: clean
# clean:
# 	rm -rf *.o

mysql: mysql.o
	g++ mysql.o -o mysql
mysql.o: mysql.cpp
	g++ -c -o mysql.o mysql.cpp
# g++ -std=c++11 -I .../include -L .../lib64 app.cc -lmysqlcppconn8 -o app