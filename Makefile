objects = main.o cJSON.o devic_list.o mqtt.o tcp_action.o config.o http.o 

main:$(objects)
	cc -o main $(objects) -lpthread -lm -lmosquitto -lcurl

main.o:typeda.h
cJSON.o:typeda.h cJSON.h
mqtt.o:typeda.h mqtt.h
tcp_action.o:typeda.h tcp_action.h
devic_list.o:typeda.h devic_list.h
config.o:typeda.h config.h
http.o:typeda.h http.h

#$(objects):typeda.h 
#cc -lpthread
#cJSON.o mqtt.o:cJSON.h
#tcp_action.o:tcp_action.h
#mqtt.o:mqtt.h
#dev_list.o:dev_list.h

clean:
	rm main *.o
