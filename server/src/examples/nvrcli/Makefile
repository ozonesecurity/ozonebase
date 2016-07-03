LIBPREFIX=/usr/local/lib/x86_64-linux-gnu/ozonebase
all:
	g++ nvrcli.cpp -g -std=gnu++11 -L$(LIBPREFIX)/base -L$(LIBPREFIX)/protocols -L$(LIBPREFIX)/libgen -L$(LIBPREFIX)/libimg -L$(LIBPREFIX)/encoders -L$(LIBPREFIX)/providers -L$(LIBPREFIX)/consumers -L$(LIBPREFIX)/processors  -lm -ljpeg -lpthread -lz -lavformat -lavcodec -lavdevice -lavutil -lswscale -ldl -lprotocols -lencoders -lprocessors -lconsumers -lproviders -lbase -limg -lgen -lm -ljpeg -lpthread -lz -lavformat -lavcodec -lavdevice -lavutil -lswscale -ldl -I/usr/local/include/ozonebase -onvrcli

clean:
	rm -f nvrcli
