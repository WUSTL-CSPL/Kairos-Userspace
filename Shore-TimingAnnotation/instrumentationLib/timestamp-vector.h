#pragma once

#include <cmath>
#include <mutex>
#include <vector>

// FOR EXPERIMENTS - Locking
#include <chrono>
#include <numeric>
extern std::vector<double> timing_tag_update_waiting_time_vector;

// A circular buffer with a fixed size that stores timestamps
template <typename T>
class TimestampVec {
   public:
    std::vector<T> buffer;
    size_t head = 0;
    size_t tail = 0;
    size_t count = 0;
    size_t maxSize;
    std::mutex mtx;  // Add mutex for thread safety

    explicit TimestampVec(size_t size) : maxSize(size), buffer(size) {
        //        mtx = std::mutex();
        buffer.reserve(size);
    }

    void lock() { mtx.lock(); }

    void unlock() { mtx.unlock(); }

    void push(const T& value) {
        //         // Lock the mutex

        /* FOR EXPERIMENTS
        auto start = std::chrono::high_resolution_clock::now();
        */

        std::lock_guard<std::mutex> lock(mtx);

        /* FOR EXPERIMENTS
        auto end = std::chrono::high_resolution_clock::now();
        timing_tag_update_waiting_time_vector.push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(end
        - start).count()/100.0);
        */

        buffer[tail] = value;
        tail = (tail + 1) % maxSize;
        if (count < maxSize) {
            ++count;
        } else {
            head = (head + 1) %
                   maxSize;  // Overwrite oldest data if buffer is full
        }
    }

    void moveFromTimestampVec(TimestampVec<T>* other, T value) {
        // No need to lock the mutex here, as the lock is done in the calling
        // function

        for (size_t i = 0; i < this->buffer.size(); ++i) {
            T tmp = other->buffer[(this->head + i) % this->maxSize];
            if (tmp > value) {
                this->push(tmp);
            }
        }
    }

    int compareTo(TimestampVec<T>* other) {
        //        std::lock_guard<std::mutex> lock(mtx); // Lock the mutex
        for (size_t i = 0; i < this->buffer.size(); ++i) {
            T tmp = other->buffer[(this->head + i) % this->maxSize];
            if (tmp > this->buffer[(this->head + i) % this->maxSize]) {
                return 1;
            } else if (tmp < this->buffer[(this->head + i) % this->maxSize]) {
                return -1;
            }
        }
        return 0;
    }

    int compareToWithSize(TimestampVec<T>* other, size_t size,
                          double threshold) {
        /* FOR EXPERIMENTS
        auto start = std::chrono::high_resolution_clock::now();
        */

        std::lock_guard<std::mutex> lock(mtx);  // Lock the mutex
        // Lock on the other buffer
        other->lock();

        /* FOR EXPERIMENTS
        auto end = std::chrono::high_resolution_clock::now();
        timing_tag_update_waiting_time_vector.push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(end
        - start).count()/100.0);
        */

        for (size_t i = 0; i < size; ++i) {
            T tmp = other->at(i);
            if (std::fabs(tmp - this->at(i)) > 2 * threshold) {
                other->unlock();
                printf("Timestamp does not match \n");
                return -1;
            }
        }
        other->unlock();
        return 0;
    }

    // T is double type
    int compareBetween(T t1, T t2, double threshold) {
        // t1 is larger than t2

        if (this->buffer.size() < 2) {
            return 0;
        }

        std::lock_guard<std::mutex> lock(mtx);

        T last = this->at(0);
        T last_interval = this->at(1) - this->at(0);

        for (size_t i = 2; i < this->buffer.size(); ++i) {
            T curr = this->at(i);

            if (curr < t2) break;

            T curr_interval = curr - last;

#ifdef Shore_DEBUG
            if (curr_interval < 0.0) {
                printf("Timestamps are not in order: curr %f vs last %f\n",
                       curr, last);
                // return -1;
            }
#endif

            T max_interval = std::max(curr_interval, last_interval);
            T min_interval = std::min(curr_interval, last_interval);


            if (max_interval > threshold) {
                // Some frames are dropped
// #ifdef Shore_DEBUG
                printf("[Warning] Frame dropped\n");
                printf("Max interval: %f, Min interval: %f\n", max_interval,
                       min_interval);
// #endif // Shore_DEBUG
                return -1;
            }

            last = curr;
            last_interval = curr_interval;
        }

        return 0;
    }

    T pop() {
        //       std::lock_guard<std::mutex> lock(mtx); // Lock the mutex

        if (count <= 0) return T();  // Should ensure there's data to pop
        T value = buffer[head];
        head = (head + 1) % maxSize;
        --count;
        return value;
    }

    // Could be removed
    T latest() {
        /* FOR EXPERIMENTS
        auto start = std::chrono::high_resolution_clock::now();
        */

        std::lock_guard<std::mutex> lock(mtx);

        /* FOR EXPERIMENTS

        auto end = std::chrono::high_resolution_clock::now();
        timing_tag_update_waiting_time_vector.push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(end
        - start).count()/100.0);
        */

        if (count <= 0) return T();  // Should ensure there's data to pop
        return buffer[(tail - 1 + maxSize) % maxSize];
    }

    T atReversed(size_t index) {
        //        std::lock_guard<std::mutex> lock(mtx); // Lock the mutex
        if (index >= count) return T();  // Should ensure there's data to pop
        // Index is stared from 0
        return buffer[(tail - (index+1) + maxSize) % maxSize];
    }

    T at(size_t index) {
        //        std::lock_guard<std::mutex> lock(mtx); // Lock the mutex
        if (index >= count) return T();  // Should ensure there's data to pop
        return buffer[(head + index) % maxSize];
    }

    bool isEmpty() const { return count == 0; }

    bool isFull() const { return count == maxSize; }

    size_t size() const { return count; }

    void clear() {
        head = 0;
        tail = 0;
        count = 0;
    }

    void resize(size_t newSize) {
        //        std::lock_guard<std::mutex> lock(mtx); // Lock the mutex
        buffer.resize(newSize);
        maxSize = newSize;
        clear();
    }

    void print() const {

        printf("Printing TimestampVec: \n");

        for (size_t i = 0; i < count; ++i) {
            std::cout << buffer[(head + i) % maxSize] << " \n";
        }
        std::cout << std::endl;
        printf("------------------------ \n");
    }
};
