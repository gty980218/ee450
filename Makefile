all: scheduler hospitalA hospitalB hospitalC client

scheduler: scheduler.cpp
	g++ -o scheduler scheduler.cpp

hospitalA: hospitalA.cpp
	g++ -o hospitalA hospitalA.cpp

hospitalB: hospitalB.cpp
	g++ -o hospitalB hospitalB.cpp

hospitalC: hospitalC.cpp
	g++ -o hospitalC hospitalC.cpp

client: client.cpp
	g++ -o client client.cpp

clean:
	rm -rf all: scheduler hospitalA hospitalB hospitalC client info.txt
