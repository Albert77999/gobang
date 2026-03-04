wsserver:wsserver.cpp
	g++ -std=c++11 $^ -o $@ -lpthread -lboost_system
json:json.cpp
	g++ -std=c++11 $^ -o $@ -lpthread -lboost_system -ljsoncpp
mysql:mysql.c 
	gcc $^ -o $@ -lmysqlclient

.PHONY:clean
clean:
	rm -rf wsserver json mysql