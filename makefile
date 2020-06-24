CFLAGS=-I/include /usr/include/mysql
LDFLAGS=-L/lib /usr/include/mysql
LDLIBS=-lbcm2835  mysqlclient

I2c: I2c.o
	g++ $(CFLAGS) $(LDFLAGSS) I2c.o $(LDLIBS) -o I2c
I2c.o: I2c.cpp
	g++ -c -o I2c.o I2c.cpp

	
.PONY: clean
clean:
	rm -rf *.o