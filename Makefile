all: serverM serverA serverQ serverP client

serverM: serverM.cpp
	g++ serverM.cpp -o serverM

serverA: serverA.cpp
	g++ serverA.cpp -o serverA

serverQ: serverQ.cpp
	g++ serverQ.cpp -o serverQ

serverP: serverP.cpp
	g++ serverP.cpp -o serverP

client: client.cpp
	g++ client.cpp -o client

clean:
	$(RM) serverM serverA serverQ serverP client