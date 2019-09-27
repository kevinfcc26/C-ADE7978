CFLAGS=-I/include
LDFLAGS=-L/lib
LDLIBS=-lbcm2835
I2c: I2c.o
	g++ $(CFLAGS) $(LDFLAGSS) I2c.o $(LDLIBS) -o I2c
I2c.o: I2c.cpp
	g++ -c -o I2c.o I2c.cpp
clean :
	rm I2c I2c.o