#include "timing-correctness.h"

#include <algorithm>
#include <fstream>
#include <map>
#include <sstream>
#include <vector>
#include <unordered_map>

// #define SHORE_DEBUG


/*
 * Some global variables
 */


std::vector<TimingVertex*>* all_timing_vertices_list;
int _graph_size = 10;  
    // This is a magic number, replace with your actual graph size
int _time_versioning_window = 20;  
    // This is a magic number, replace with your actual versioning window
int shoreline_index = 0;


// Hash table to store function pointers
std::unordered_map<int, void*> input_to_callback_ptr_map;

// dependee -> dependents
std::map<int, std::vector<int>>* _dependency_map_by_vertex_id;


void setGraphSize(int graph_size) { 
    _graph_size = graph_size; 
};

void initTimingGraph() {
    _dependency_map_by_vertex_id = new std::map<int, std::vector<int>>;

    const std::string graph_file_name =
        "/tmp/vertex_dependency_map.txt";  // Replace with your actual file name

    readNumbersFromFile(graph_file_name, _dependency_map_by_vertex_id);

    all_timing_vertices_list = new std::vector<TimingVertex*>;

    printf("[Shore-Debug] Graph size is %d\n", _graph_size);

    // First, initialize all vertices
    for (int id = 0; id < _graph_size; id++) {
        // TODO fix the shoreline ID, automatically generate the Shoreline ID

        TimingVertex* new_vertex =
            new TimingVertex(id, _time_versioning_window);
        all_timing_vertices_list->push_back(new_vertex);
    }

    // Second, for the vertices do not have any dependents, set them as input
    for (int id = 0; id < _graph_size; id++) {
        TimingVertex* vertex = all_timing_vertices_list->at(id);

        if (_dependency_map_by_vertex_id->find(id) ==
            _dependency_map_by_vertex_id->end()) {
            // Disable this for testing
            vertex->setInputFlag(true);
            vertex->def_sources.push_back(
                new TimestampVec<double>(_time_versioning_window));
        }
    }

    // Third, add the dependency if needed
    for (std::map<int, std::vector<int>>::iterator it =
             _dependency_map_by_vertex_id->begin();
         it != _dependency_map_by_vertex_id->end(); ++it) {
        int dependee_id = it->first;
        std::vector<int> dependents_ids = it->second;

        TimingVertex* dependee_vertex =
            all_timing_vertices_list->at(dependee_id);

        for (int dependent_id : dependents_ids) {
            TimingVertex* dependent_vertex =
                TimingVertex::fetchTimingVertexByID(all_timing_vertices_list,
                                                    dependent_id);
            dependee_vertex->addPrecVertex(dependent_vertex);
        }
#ifdef SHORE_DEBUG
        printf("[Shore-User] Node %d has %d dependents\n", dependee_id,
               dependee_vertex->getNumOfPrecVertex());
#endif  // SHORE_DEBUG
    }

#ifdef SHORE_DEBUG
    printf("Timing graph is initialized with size %d\n", _graph_size);
#endif  // SHORE_DEBUG

    return;
};


void registerROSCallbackFunctionPtr(int id, void* func) {

    // printf("Registering callback function for vertex: %d and the addr %p \n", id, func);

    TimingVertex* vertex = TimingVertex::fetchTimingVertexByID(
        all_timing_vertices_list, id);

    vertex->_shoreline_info->setROSCallbackFuncPtr(func);

    input_to_callback_ptr_map[id] = func;
}

void readNumbersFromFile(const std::string& file_name,
                         std::map<int, std::vector<int>>* dependency_map) {
    if (dependency_map == nullptr) {
        printf("Dependency map is not initialized\n");
    }

    std::ifstream file(file_name);

    if (!file.is_open()) {
        printf("Failed to open file\n");
        return;
    }

    std::string line;
    if (std::getline(file, line)) {
        std::istringstream iss(line);
        int dependee_id;
        char arrow;
        iss >> dependee_id >> arrow >> arrow;

        std::vector<int> dependents_ids;
        int num;

        while (iss >> num) {
            dependents_ids.push_back(num);
        }

        dependency_map->insert(
            std::pair<int, std::vector<int>>(dependee_id, dependents_ids));

        file.close();
    }

#ifdef SHORE_DEBUG
    // Print debug information

        for (const auto& entry : *_dependency_map_by_vertex_id) {
            int dependee_id = entry.first;
            const std::vector<int>& dependents_ids = entry.second;

            std::cout << "Dependee ID: " << dependee_id << std::endl;
            std::cout << "Dependents IDs: ";
            for (int dependent_id : dependents_ids) {
                std::cout << dependent_id << " ";
            }
            std::cout << std::endl;
        }
#endif  // SHORE_DEBUG

    return;
}

TimingVertex::TimingVertex(int id, int timing_version_size) {
    
    _id = id;

    uses = new TimestampVec<double>(timing_version_size);
    defs = new TimestampVec<double>(timing_version_size);

    // FOR EXPERIMENTS
    /*
    TimestampVec<double> memory_consumption_test(10);

    size_t vectorObjectSize = sizeof(memory_consumption_test);
    size_t sizeOfElements =
        sizeof(double) * memory_consumption_test.buffer.capacity();
    size_t totalMemory = vectorObjectSize + sizeOfElements;

    printf("------------------------------------\n");
    printf("Memory consumption of Timestamp Struct is %lu bytes\n",
           sizeof(TimestampVec<double>));
    printf("Memory consumption of vector object is %lu bytes\n",
           vectorObjectSize);
    printf("Memory consumption of double is %lu bytes\n", sizeof(double));
    printf("Memory consumption of vector elements is %lu bytes\n",
           sizeOfElements);
    printf("Memory consumption of vector is %lu bytes\n", totalMemory);
    printf("The size of size_t is %lu bytes\n", sizeof(size_t));
    printf("The size of mutex is %lu bytes\n", sizeof(std::mutex));
    printf("------------------------------------\n");
    */

    _shoreline_info = new Shoreline_Info(shoreline_index++);
    _policy_handler = new TCViolationHandler();

    _shoreline_info->setVertexID(id);
    _policy_handler->setShorelineInfo(_shoreline_info);

    versioning_size = timing_version_size;
    is_input = false;
};

TimingVertex::~TimingVertex() {
    delete uses;
    delete defs;
    for (int i = 0; i < def_sources.size(); i++) {
        delete def_sources[i];
    }
};

std::vector<TimingVertex*>* TimingVertex::initTimingGraph(
    int graph_size, int timing_versioning_window) {
    // Load the timing graph from a text
    std::vector<TimingVertex*>* res = new std::vector<TimingVertex*>;
    res->reserve(graph_size);
    for (int i = 0; i < graph_size; i++) {
        // TODO fix the shoreline ID, automatically generate the Shoreline ID
        TimingVertex* new_vertex = new TimingVertex(i, _time_versioning_window);
        res->push_back(new_vertex);
    }
    return res;
};

TimingVertex* TimingVertex::fetchTimingVertexByID(
    std::vector<TimingVertex*>* timing_vertex_list, int id) {
    if (timing_vertex_list->size() <= id) {
        printf("Vertex ID %d is out of range\n", id);
        return nullptr;
    }

    return timing_vertex_list->at(id);
};

void TimingVertex::setInputFlag(bool is_input) { this->is_input = is_input; };

int TimingVertex::checkDefNumberEqualsToPrecVertices() {
    int def_sources_size = this->def_sources.size();
    int prec_def_sources_size = 0;

    for (int i = 0; i < this->getNumOfPrecVertex(); i++) {
        prec_def_sources_size += this->_prec_vertexes[i]->def_sources.size();
    }

#ifdef SHORE_DEBUG
    printf("The def sources size is %d\n", def_sources_size);
    printf("The prec def sources size is %d\n", prec_def_sources_size);
#endif  // SHORE_DEBUG

    // This is an input vertex
    prec_def_sources_size =
        prec_def_sources_size == 0 ? 1 : prec_def_sources_size;

    return def_sources_size == prec_def_sources_size;
}

int TimingVertex::addPrecVertex(TimingVertex* prec_vertex) {
    _prec_vertexes.push_back(prec_vertex);

    // Check the size of prec vertex def sources
    int prec_vertex_def_sources_size = prec_vertex->def_sources.size();

    for (int i = 0; i < prec_vertex_def_sources_size; i++) {
        TimestampVec<double>* new_def_source =
            new TimestampVec<double>(this->versioning_size);
        this->def_sources.push_back(new_def_source);
        def_sources_added_nums.push_back(0);
    }

    this->checkDefNumberEqualsToPrecVertices();
    /*
        assert(this->def_sources.size() == this->_prec_vertexes.size());
        assert(this->def_sources.size() == this->def_sources_added_nums.size());
    */
    return def_sources.size();
}

int TimingVertex::addPrecVertexByID(
    int id, std::vector<TimingVertex*>* timing_vertex_list) {
    TimingVertex* prec_vertex =
        TimingVertex::fetchTimingVertexByID(timing_vertex_list, id);
    _prec_vertexes.push_back(prec_vertex);

    // Check the size of prec vertex def sources
    int prec_vertex_def_sources_size = prec_vertex->def_sources.size();

    for (int i = 0; i < prec_vertex_def_sources_size; i++) {
        TimestampVec<double>* new_def_source =
            new TimestampVec<double>(this->versioning_size);
        this->def_sources.push_back(new_def_source);
        def_sources_added_nums.push_back(0);
    }

    /*
        assert(this->def_sources.size() == this->_prec_vertexes.size());
        assert(this->def_sources.size() == this->def_sources_added_nums.size());
    */

    return def_sources.size();
}

void TimingVertex::updateTimingVertex(double sensor_timestamp) {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    double time_point = std::chrono::duration<double>(duration).count();

    ////////////////////////////////////
    // Update `def` timestamp
    ////////////////////////////////////
    // if (this->is_input) {
    if (this->_prec_vertexes.size() == 0) {
        // FIXME: use the timestamp in sensor message

#ifdef SHORE_DEBUG
        printf(
            "[Timing Update] Vertex %d is updated with def time from sensor "
            "timestamp is %f \n",
            this->id, sensor_timestamp);
#endif  // SHORE_DEBUG

        // In our testing scenario, the data is recorded and replayed, thus we
        // use the current time as def time
        this->defs->push(sensor_timestamp);
    } else {
        // For non-input vertex, the def time is the latest def time of its
        for (int i = 0; i < _prec_vertexes.size(); i++) {
            // FIXME: update to the latest version

            if (_prec_vertexes[i]->def_sources.size() == 1) {
                TimestampVec<double>* tmp_defs =
                    _prec_vertexes[i]->def_sources[0];

                double laset_def_time =
                    this->def_sources[i]
                        ->latest();  // Fix: Replace get_lastest_def_timestamp()
                                     // with latest()
                this->def_sources[i]->moveFromTimestampVec(tmp_defs,
                                                           laset_def_time);
            }
            // TODO the precedessor has two def vectors
            // this->def_sources.size() = sum the def_sources of _prec_vertexes
        }
    }

    ////////////////////////////////////
    // Update `use` timestamp
    ////////////////////////////////////
    this->uses->push(time_point);
#ifdef SHORE_DEBUG
    printf("[Timing Update] Vertex %d is updated with use time %f\n", this->id,
           this->uses->latest());
#endif  // SHORE_DEBUG
    return;
};

// For EXPERIMENTS
// extern std::vector<double> durations;
void TimingVertex::updateDefTiming(double sensor_timestamp) {
#ifdef SHORE_DEBUG
    printf("The def source is updated with vertex %d\n", this->getVertexID());
#endif  // SHORE_DEBUG

    /* FOR EXPERIMENTS
    auto start = std::chrono::high_resolution_clock::now();
    */

    if (std::abs(sensor_timestamp - 0.0) >
        std::numeric_limits<double>::epsilon()) {
        //   if (vertex->isInput()) {

        // In this case, only one def source
        this->def_sources.front()->push(sensor_timestamp);
    } else {
        auto now = std::chrono::system_clock::now();
        auto duration = now.time_since_epoch();
        double time_point = std::chrono::duration<double>(duration).count();
        this->def_sources.front()->push(time_point);
    }

    /* FOR EXPERIMENTS
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end -
    start); durations.push_back(duration.count());

    if (durations.size() == 10 || durations.size() == 20 ||
        durations.size() == 50 || durations.size() == 100 ||
        durations.size() == 200 || durations.size() == 500) {
        double sum = 0;
        for (int i = 0; i < durations.size(); i++) {
            sum += durations[i];
        }

        std::cout << "[Shore-Experiments] Consumed CPU time: " << sum <<
    std::endl;
    }
    */
}

void TimingVertex::updateUseTiming() {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    double time_point = std::chrono::duration<double>(duration).count();
    this->uses->push(time_point);
}

void TimingVertex::resetUpdateTimingVertex() {
    // Remove the USE timestamp
    this->uses->pop();

    // Remove the DEF timestamp
    for (int i = 0; i < this->def_sources_added_nums.size(); i++) {
        for (int j = 0; j < this->def_sources_added_nums[i]; j++) {
            this->def_sources[i]->pop();
        }
    }
};

// Timing Correctness

TimingCorrectness::TimingCorrectness(enum TimingConstraintType type,
                                     double threshold,
                                     enum HandlingPolicy violation_policy)
    : _type(type), _threshold(threshold) {
    // Initialize the timing correctness object
    _policy_handler.setPolicy(violation_policy);

#ifdef SHORE_DEBUG
    printf(
        "Timing correctness object is initialized with type %d, threshold %f, "
        "and policy %d\n",
        type, threshold, violation_policy);
#endif  // SHORE_DEBUG

    _vertex = new TimingVertex(0, 10);  
        // FIXME: hardcode the size of timestamp vector
};

bool TimingVertex::checkFreshness(double threshold) {
    double data_age = 0;
    double curr_data_age = 0;
    for (int i = 0; i < this->_prec_vertexes.size(); i++) {
        curr_data_age = this->_prec_vertexes[i]->get_lastest_def_timestamp() -
                        this->get_lastest_use_timestamp();
        data_age = std::max(data_age, curr_data_age);
    }
    if (data_age > threshold) {
        printf("Data age is %f, which is greater than the threshold %f\n",
               data_age, threshold);
        // _policy_handler.handle();
        return true;
    }
    return false;
}

bool TimingVertex::checkConsistency(double threshold) {
    if (this->def_sources.size() < 2) {
        printf("Consistency check requires at least two def sources\n");
        return false;
    }

#ifdef SHORE_DEBUG
    printf("Consistency is checked at node %d\n", this->id);
#endif  // SHORE_DEBUG

    // Check consistency
    double max_temporal_consistency = 0.0;
    double avg_temporal_consistency = 0.0;

    for (int i = 0; i < this->def_sources.size(); i++) {
        for (int j = i + 1; j < this->def_sources.size(); j++) {
            TimestampVec<double>* defs_src_1 = this->def_sources[i];
            TimestampVec<double>* defs_src_2 = this->def_sources[j];

            // Check if there are more than 2 frames in each source
            if (defs_src_1->size() < 2 || defs_src_2->size() < 2) {
                return false;
            }

            // Get the frequency larger one
            double interval_src_1 = defs_src_1->at(0) - defs_src_1->at(1);
            double interval_src_2 = defs_src_2->at(0) - defs_src_2->at(1);

            // We swap the sources to simplify the comparison
            if (interval_src_1 < interval_src_2) {
                TimestampVec<double>* tmp = defs_src_1;
                defs_src_1 = defs_src_2;
                defs_src_2 = tmp;
            }

            if (fabs(interval_src_1 - interval_src_2) <
                std::min(interval_src_1, interval_src_2)) {
                // The two sources might have the same frequencies
                // TODO replace this by a frequency check
                int ret =
                    defs_src_1->compareToWithSize(defs_src_2, 2, threshold);
                if (ret == -1) {
                    printf("[Warning] Temporal consistency is violated\n");
                    return true;
                } else {
                    return false;
                }
            } else {
                // The two sources have different frequencies
                int ret = defs_src_2->compareBetween(
                    defs_src_1->at(0), defs_src_1->at(1), threshold);
                if (ret == -1) {
                    printf("[Warning] Temporal consistency is violated\n");
                    return true;
                } else {
                    return false;
                }
            }
        }
    }

    return false;
}

bool TimingVertex::checkStability(double threshold) {
    if (this->stability_timestamp == 0) {
        // Check STABILITY via USE timestamps

        std::vector<double> intervals;

        intervals.reserve(this->uses->size() - 1);

        for (int i = 0; i < this->uses->size() - 1; i++) {
            double interval = this->uses->buffer[i + 1] - this->uses->buffer[i];
            intervals.push_back(interval);
        }

        if (intervals.empty()) {
            return false;
        }

        double max_interval =
            *std::max_element(intervals.begin(), intervals.end());

        double min_interval =
            *std::min_element(intervals.begin(), intervals.end());

        double jitters = max_interval - min_interval;

        if (jitters > _threshold || min_interval < 0) {
            // min_interval < 0 indicates that the data is not consumed in
            // order
            return true;
        }

    } else {
        // Check STABILITY via DEF timestamps
        for (int i = 0; i < this->def_sources.size(); i++) {
            std::vector<double> intervals;
            TimestampVec<double>* tmp_defs = this->def_sources[i];
            intervals.reserve(tmp_defs->size() - 1);

            for (int i = 0; i < tmp_defs->size() - 1; i++) {
                double interval = tmp_defs->buffer[i + 1] - tmp_defs->buffer[i];
                intervals.push_back(interval);
            }

            double max_interval =
                *std::max_element(intervals.begin(), intervals.end());
            double min_interval =
                *std::min_element(intervals.begin(), intervals.end());

            double jitters = max_interval - min_interval;

            if (jitters > _threshold || min_interval < 0) {
                // min_interval < 0 indicates that the data is not consumed
                // in order
#ifdef SHORE_DEBUG
                printf("Stability is violated, jitters is %f\n", jitters);
#endif  // SHORE_DEBUG
                return true;
            }
        }
    }

    return false;
}

// The return indicate if the timing correctness is violated
bool TimingVertex::checkTimingCorrectness(TimingConstraintType property,
                                          double threshold) {
    this->_type = property;
    this->_threshold = threshold;
    if (this->_type == TimingConstraintType::TYPE_FRESHNESS) {
        // Check freshness
        return this->checkFreshness(threshold);
    } else if (this->_type == TimingConstraintType::TYPE_CONSISTENCY) {
        return this->checkConsistency(threshold);

    } else if (this->_type == TimingConstraintType::TYPE_STABILITY) {
        return this->checkStability(threshold);
    }
    return false;
}

// This is the INTERFACE call for LLVM pass
// (int id, TimingConstraintType property, double threshold, HandlingPolicy
// violation_policy)
int log_loop = 0;
int checkTimingCorrectnessByID(int id, int property, double threshold,
                               int violation_policy) {
    int ret = 0;

    if (id < 0 || id >= all_timing_vertices_list->size()) {
        return ret;
    }

    TimingVertex* vertex =
        TimingVertex::fetchTimingVertexByID(all_timing_vertices_list, id);

    if (vertex == nullptr) {
        printf("Vertex is not found\n");
        return ret;
    }

    if (log_loop++ > 300) {
        log_loop = 0;

        for (int i = 0; i < _graph_size; i++)
        {
            TimingVertex* log_vertex = all_timing_vertices_list->at(i);
            log_vertex->printTiming();
            /* code */
        }
    }

#ifdef SHORE_DEBUG
    printf(
        "checkTimingCorrectnessByID() is called with parameters: id %d, "
        "property "
        "%d, threshold %f, violation_policy %d\n",
        id, property, threshold, violation_policy);


#endif  // SHORE_DEBUG

    vertex->updateTimingVertex();

    bool is_violated = vertex->checkTimingCorrectness(
        (TimingConstraintType)property, threshold);

    TCViolationHandler* handler = vertex->getPolicyHandler();

    is_violated = 1;

    if (is_violated) {
        handler->setPolicy(static_cast<HandlingPolicy>(violation_policy));
        ret = handler->handle();

        if (handler->isAborting()) {
            vertex->resetUpdateTimingVertex();
            ret = 1;
        }
    } else {
        if (handler->isPrioritizing()) {
            handler->resetPriority();
        }
    }

    return ret;
}

int updateDefTimingByID(int id) {
    int ret = 0;

    if (id < 0 || id >= all_timing_vertices_list->size()) {
        return ret;
    }

    TimingVertex* vertex =
        TimingVertex::fetchTimingVertexByID(all_timing_vertices_list, id);

    if (vertex == nullptr) {
        printf("Vertex is not found\n");
        return ret;
    }

    vertex->updateDefTiming(0.0);

    return ret;
}

int updateInputDefTimeByID(int id, void* sensor_input_ptr) {
    // Check input condition
    int ret = 0;

    if (id < 0 || id >= all_timing_vertices_list->size()) {
        return -1;
    } else if (sensor_input_ptr == nullptr) {
        return -1;
    }

    /*
     * Parse the timestamp from the sensor input
     */

    void** sensor_input_ptr_ptr = (void**)sensor_input_ptr;

    Shore_Header* shore_header = (Shore_Header*)(*sensor_input_ptr_ptr);

    double sensor_timestamp = shore_header->timestampInSec();

#ifdef SHORE_DEBUG
    printf("The input vertex id is %d\n", id);
    printf("Sensor timestamp Shore: %f\n", sensor_timestamp);
#endif
    /*
     * Get the vertex by ID and then update the timestamp
     */

    TimingVertex* vertex =
        TimingVertex::fetchTimingVertexByID(all_timing_vertices_list, id);

    if (vertex == nullptr) {
        printf("Vertex is not found\n");
        return ret;
    }

    vertex->updateDefTiming(sensor_timestamp);
    vertex->updateUseTiming();

    return 0;
}

int updateUseTimingByID(int id) {
    int ret = 0;

    if (id < 0 || id >= all_timing_vertices_list->size()) {
        return ret;
    }

    TimingVertex* vertex =
        TimingVertex::fetchTimingVertexByID(all_timing_vertices_list, id);

    if (vertex == nullptr) {
        printf("Vertex is not found\n");
        return ret;
    }

    vertex->updateUseTiming();

    return ret;
}


// For debugging

void TimingVertex::printTiming() {
    printf("------------------------------------\n");
    printf("Vertex ID: %d\n", this->_id);
    if (this->hasPrecVertex() || this->is_input) {\
        printf("Current Def Timestamp is : ");
        if (this->def_sources.size() == 0) {
            printf("N/A \n");
        }
        for (int i = 0; i < this->def_sources.size(); i++)
        {
            printf("%f ｜ ", this->def_sources[i]->latest());
        }
        printf("\n");
    }

    if (this->uses->latest() != 0.0) {
        printf("Current Use Timestamp is : %f \n", this->uses->latest());
    }
    printf("------------------------------------\n");
}


void TimingVertex::printTimingVertex() {
    printf("------------------------------------\n");
    printf("Vertex ID: %d\n", this->_id);
    printf("Shoreline ID: %d\n", this->_shoreline_info->shoreline_id);
    printf("Versioning Size: %d\n", this->versioning_size);
    printf("Is Input: %d\n", this->is_input);
    printf("Prec Vertexes: %zu\n", this->_prec_vertexes.size());
    printf("Succ Vertexes: %zu\n", this->_succ_vertexes.size());
    printf("Defs: %f\n", this->defs->latest());
    printf("Uses: %f\n", this->uses->latest());
    printf("Def Sources: %zu\n", this->def_sources.size());
    printf("Def Sources Added Nums: %zu\n",
           this->def_sources_added_nums.size());
    printf("Policy: %d\n", this->getPolicy());
    printf("Type: %d\n", this->_type);
    printf("Threshold: %f\n", this->_threshold);
    printf("------------------------------------\n");
    return;
}