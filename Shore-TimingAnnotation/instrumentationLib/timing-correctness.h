#pragma once

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <map>
#include <vector>

#include "Shore-common.h"
#include "handling-policy.h"
#include "timestamp-vector.h"

enum TimingConstraintType {
    TYPE_NONE = 0,
    TYPE_FRESHNESS = 1,
    TYPE_CONSISTENCY = 2,
    TYPE_STABILITY
    // User may extend more
};

// Need to add extern C to make the function linkable
extern "C" {
void setGraphSize(int graph_size);
void initTimingGraph();
int checkTimingCorrectnessByID(int id, int property, double threshold,
                               int violation_policy);
int updateDefTimingByID(int id);
int updateUseTimingByID(int id);
int updateInputDefTimeByID(int id, void* sensor_input_ptr);

// Utility function
void readNumbersFromFile(const std::string& file_name,
                         std::map<int, std::vector<int>>* dependency_map);
}



/**
 * @class TimingVertex
 * @brief Represents a vertex in the timing graph used for timing correctness checking.
 * 
 * The TimingVertex class stores information about a vertex in the timing graph, including its ID, versioning size, input flag, predecessor and successor vertices,
 * timestamp vectors for definitions and uses, timing constraint type, threshold, violation handler, stability timestamp, and shoreline information.
 * It also provides various methods for manipulating and accessing the vertex's properties, such as adding predecessor vertices, updating timing information,
 * checking correctness properties, and setting handling policies.
 */
class TimingVertex {
   public:
    /*
    * Variables
    */

    int _id;
    int versioning_size;
    bool is_input;  // For input vertex, the def should be set from sensor
                    // message

    std::vector<TimingVertex*> _prec_vertexes;
    std::vector<TimingVertex*> _succ_vertexes;

    TimestampVec<double>* defs;  // REMOVE

    std::vector<TimestampVec<double>*> def_sources;
    std::vector<int>
        def_sources_added_nums;  // use this to reset the timestamps

    TimestampVec<double>* uses;

    // For timing correctness checking
    enum TimingConstraintType _type;
    double _threshold;
    TCViolationHandler* _policy_handler;

    // 0 for USE, 1 for DEF
    int stability_timestamp = 0;

    struct Shoreline_Info* _shoreline_info;


    /*
    * Gloabal fucntions
    */

    static std::vector<TimingVertex*>* initTimingGraph(
        int graph_size, int timing_versioning_window);
    static TimingVertex* fetchTimingVertexByID(
        std::vector<TimingVertex*>* timing_vertex_list, int id);
    /*
    * Local fucntions
    */

    TimingVertex(int id, int timing_version_size = 10);
    ~TimingVertex();

    void setInputFlag(bool is_input);
    int checkDefNumberEqualsToPrecVertices();

    int addPrecVertex(TimingVertex* prec_vertex);
    int addPrecVertexByID(int id,
                          std::vector<TimingVertex*>* timing_vertex_list);
    int getNumOfPrecVertex() { return _prec_vertexes.size(); }

    bool hasPrecVertex() { return _prec_vertexes.size() > 0; }

    void updateTimingVertex(double sensor_timestamp = 0.0);
    void updateDefTiming(double sensor_timestamp);
    void updateUseTiming();
    void resetUpdateTimingVertex();
    double get_lastest_def_timestamp() { return defs->latest(); }
    double get_lastest_use_timestamp() { return uses->latest(); }
    double get_def_timestamp(int index) { return defs->buffer[index]; }
    int getVertexID() { return _id; }

    // For debugging
    void printTiming();
    void printTimingVertex();

    // FIXME
    bool CONSISTENCY();
    bool FRESHNESS();
    bool STABILITY();

    bool checkFreshness(double threshold);
    bool checkConsistency(double threshold);
    bool checkStability(double threshold);

    bool checkTimingCorrectness(
        TimingConstraintType property = TimingConstraintType::TYPE_FRESHNESS,
        double threshold = 0.5);

    TCViolationHandler* getPolicyHandler() { return _policy_handler; }
    void setHandlingPolicy(HandlingPolicy policy) {
        _policy_handler->setPolicy(policy);
    }
    HandlingPolicy getPolicy() { return _policy_handler->_policy; }
};

// TO REMOVE
class TimingCorrectness {
   public:
    TimingCorrectness();
    TimingCorrectness(enum TimingConstraintType type, double threshold,
                      enum HandlingPolicy violation_policy);

    ~TimingCorrectness();

    TimingVertex* _vertex;

    enum TimingConstraintType _type;
    double _threshold;
    TCViolationHandler _policy_handler;

    // FIXME
    bool CONSISTENCY();
    bool FRESHNESS();
    bool STABILITY();

    bool checkTimingCorrectness(
        TimingConstraintType property = TimingConstraintType::TYPE_FRESHNESS,
        double threshold = 0.5);
};