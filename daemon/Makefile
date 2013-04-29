FLAGS=-DOSX -I.. -O2
OBJECTS=ThinIPC.o contextiso.o Hypervisor.o Virtualbox.o

daemon: daemon.cpp $(OBJECTS)
	g++ $(FLAGS) -o daemon $(OBJECTS) daemon.cpp -lcurl -lssl -lcrypto

contextiso.o: ../contextiso.cpp ../contextiso.h 
	g++ $(FLAGS) -c -o contextiso.o ../contextiso.cpp

Hypervisor.o: ../Hypervisor.cpp ../Hypervisor.h
	g++ $(FLAGS) -c -o Hypervisor.o ../Hypervisor.cpp

Virtualbox.o: ../Virtualbox.cpp ../Virtualbox.h ../Hypervisor.h
	g++ $(FLAGS) -c -o Virtualbox.o ../Virtualbox.cpp

ThinIPC.o: ../ThinIPC.cpp ../ThinIPC.h
	g++ $(FLAGS) -c -o ThinIPC.o ../ThinIPC.cpp

clean:
	rm -f $(OBJECTS)
