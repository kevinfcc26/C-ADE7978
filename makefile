CFLAGS=-I/include -I/usr/include/mysql
LDFLAGS=-L/lib -L/usr/include/mysql
LDLIBS=-lbcm2835 -l mysqlclient

I2c: I2c.o
	g++ $(CFLAGS) $(LDFLAGSS) I2c.o $(LDLIBS) -o I2c
I2c.o: I2c.cpp
	g++ -c -o I2c.o I2c.cpp $(CFLAGS) $(LDFLAGSS) $(LDLIBS)

	
.PONY: clean
clean:
	rm -rf *.o