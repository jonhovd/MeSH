# Based on Makefile from <URL: http://hak5.org/forums/index.php?showtopic=2077&p=27959 >

PROGRAM = MeSHWeb

############# Main application #################
all:    $(PROGRAM)
.PHONY: all

# source files
#DEBUG_INFO = YES
SOURCES = $(shell find -L . -name '*.cpp'|grep -v "/example/")
OBJECTS = $(SOURCES:.cpp=.o)
DEPS = $(OBJECTS:.o=.dep)

######## compiler- and linker settings #########
CXX = g++
CXXFLAGS = -I/usr/local/include -I/usr/include -Icpp-elasticsearch/src -W -Wall -Werror -pipe -std=c++17
ifdef DEBUG_INFO
 CXXFLAGS += -g
 LIBSFLAGS = -L/usr/lib/debug/usr/lib
else
 CXXFLAGS += -O3
 LIBSFLAGS = -L/usr/lib
endif
LIBSFLAGS += -lwt -lwthttp -lboost_system -lboost_locale -lpthread

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -o $@ -c $<

%.dep: %.cpp
	$(CXX) $(CXXFLAGS) -MM $< -MT $(<:.cpp=.o) > $@

############# Main application #################
$(PROGRAM):	$(OBJECTS) $(DEPS)
	$(CXX) -o $@ $(OBJECTS) $(LIBSFLAGS)

################ Dependencies ##################
ifneq ($(MAKECMDGOALS),clean)
include $(DEPS)
endif

################### Clean ######################
clean:
	find . -name '*~' -delete
	-rm -f $(PROGRAM) $(OBJECTS) $(DEPS)

install:
	sudo cp strings.xml /opt/Helsebib/MeSHWeb/ && \
        sudo cp wt_config.xml /opt/Helsebib/MeSHWeb/ && \
        sudo cp MeSH.css /opt/Helsebib/MeSHWeb/ && \
        sudo rm -rf /opt/Helsebib/MeSHWeb/images && sudo cp -ar images /opt/Helsebib/MeSHWeb/ && \
        sudo cp MeSHWeb_ssl.sh /opt/Helsebib/MeSHWeb/ && \
        sudo cp startup-script.sh /etc/init.d/MeSHWeb.sh && sudo chmod 755 /etc/init.d/MeSHWeb.sh && \
        strip -s $(PROGRAM) && \
        sudo cp $(PROGRAM) /opt/Helsebib/MeSHWeb/
