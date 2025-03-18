CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra
BOOST_PATH = ../libs/boost_1_82_0 

# Si vous avez installé Boost localement, utilisez cette ligne
# BOOST_PATH = $(HOME)/local/boost_1_82_0

INCLUDES = -I$(BOOST_PATH)

# Si vous avez installé Boost localement, vous pourriez avoir besoin de ceci
# LDFLAGS = -L$(BOOST_PATH)/stage/lib
# LDLIBS = -lboost_system -lboost_graph

all: graph_analysis

graph_analysis: source/main.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $< $(LDFLAGS) $(LDLIBS)

clean:
	rm -f graph_analysis