#include <vector>
#include <random>
#include <utility>
#include <chrono>
#include <unordered_set>
#include <iostream>
#include <optional>
#include <functional>

const uint64_t kBigPrime = 2000000011;

class LinearHashFunction {
public:
    LinearHashFunction(uint64_t first_coeff, uint64_t second_coeff) {
        first_coeff_ = first_coeff;
        second_coeff_ = second_coeff;
    }

    uint32_t GetElementHash(int32_t element) const {
        if (element < 0) {
            return (first_coeff_ * ((element + bigprime_) % bigprime_) +
                    second_coeff_) % bigprime_;
        }
        return (first_coeff_ * element + second_coeff_) % bigprime_;
    }

private:
    uint64_t first_coeff_;
    uint64_t second_coeff_;
    uint64_t bigprime_ = kBigPrime;
};

LinearHashFunction MakeRandomLinearHashFunction(std::mt19937* gen);

LinearHashFunction MakeRandomLinearHashFunction(std::mt19937* gen) {
    uint64_t prime = kBigPrime;
    std::uniform_int_distribution<int64_t> dist(0, prime - 1);
    uint64_t firts_coeff = static_cast<uint32_t>(dist(*gen));
    uint64_t second_coeff = static_cast<uint32_t>(dist(*gen));
    return {firts_coeff, second_coeff};
}

class PolinomialHashTableSecondLevel {
public:
    PolinomialHashTableSecondLevel() {
    }

    PolinomialHashTableSecondLevel(std::mt19937* gen, const std::vector<int32_t>& backet) {
        MakeSecondLevelHashTable(gen, backet);
    }

    bool Contains(int32_t element) const {
        if (hash_table_second_level_.empty()) {
            return false;
        }
        size_t element_hash = (hash_function_.GetElementHash(element)) %
                              hash_table_second_level_.size();
        if (!hash_table_second_level_[element_hash]) {
            return false;
        } else {
            return (*hash_table_second_level_[element_hash]) == element;
        }
    }

private:
    LinearHashFunction hash_function_ = LinearHashFunction(0, 0);
    std::vector<std::optional<int32_t>> hash_table_second_level_;
    static const size_t kConstForBacketSize = 4;

    bool CollisionAppeared(int32_t element, size_t backet_size) const {
        size_t element_hash = hash_function_.GetElementHash(element) % backet_size;
        return (hash_table_second_level_[element_hash] &&
                *(hash_table_second_level_[element_hash]) != element);
    }

    bool SplitedIntoBacketsWithCollisions(const std::vector<int32_t>& backet) {
        size_t second_level_hashtable_size =
                kConstForBacketSize * backet.size() * backet.size();

        for (const auto& element: backet) {
            // if current second-level hash-table contains not current element and
            // not end_set_pointer => have collisions
            if (CollisionAppeared(element, second_level_hashtable_size)) {
                return true;
            } else {
                hash_table_second_level_[hash_function_.GetElementHash(element) %
                                         second_level_hashtable_size] = element;
            }
        }
        return false;
    }

    void MakeSecondLevelHashTable(std::mt19937* gen, const std::vector<int32_t>& backet) {
        if (backet.empty()) {
            return;
        }
        hash_table_second_level_.resize(backet.size());

        size_t second_level_hashtable_size =
                kConstForBacketSize * backet.size() * backet.size();
        hash_table_second_level_.resize(second_level_hashtable_size);
        do {
            hash_function_ = MakeRandomLinearHashFunction(gen);
        } while (SplitedIntoBacketsWithCollisions(backet));
    }
};

// first-level hash table
class FixedSet {
public:
    // Constructor, which recieves random numbers generator and set of elements
    // Creates first-level hash table for set of recieved elements
    void Initialize (const std::vector<int32_t>& elements) {
        hash_table_.resize((elements).size());
        MakeFirstLevelHashTable(elements);
    }

    bool Contains(int32_t element) const {
        if (hash_table_.empty()) {
            return false;
        }
        return hash_table_[(hash_function_first_level_.GetElementHash(element)) %
                           hash_table_.size()].Contains(element);
    }

private:
    LinearHashFunction hash_function_first_level_ = LinearHashFunction(0, 0);
    std::vector<PolinomialHashTableSecondLevel> hash_table_;
    static const size_t kConstForBacketSize = 4;

    // variable for sum of squared sizes of backets
    uint64_t CountSumSquaredBacketSizes(const std::vector<size_t>& backet_sizes) const {
        uint64_t  sum_backet_sizes_squared = 0;
        for (const auto& size: backet_sizes) {
            sum_backet_sizes_squared += size * size;
        }
        return sum_backet_sizes_squared;
    }

    std::vector<size_t> SplitIntoBackets(const std::vector<int32_t>& elements) {
        // vector of backets sizes
        std::vector<size_t> backet_sizes(elements.size(), 0);

        size_t set_size = elements.size();

        // loop for increasing backets sizes
        for (const auto& element: elements) {
            backet_sizes[hash_function_first_level_.GetElementHash(element) % set_size] += 1;
        }
        return backet_sizes;
    }

    void MakeFirstLevelHashTable(const std::vector<int32_t>& elements) {
        std::mt19937 gen;
        if (elements.empty()) {
            return;
        }

        // vector for backets
        std::vector<std::vector<int32_t>> hash_table_first_level(elements.size());

        do {
            // generating coefficients of linear hash function
            hash_function_first_level_ = MakeRandomLinearHashFunction(&gen);

            // Increasing backets sizes and count sum of their squared sizes
        } while (CountSumSquaredBacketSizes(SplitIntoBackets(elements)) >=
                 kConstForBacketSize * elements.size());

        // filling first level hash table with elements of set
        for (const auto& element: elements) {
            hash_table_first_level[hash_function_first_level_.GetElementHash(element) %
                                   hash_table_.size()].push_back(element);
        }
        size_t i_index = 0;

        // creating second-level hash table and filling it with elements
        for (const auto& backet: hash_table_first_level) {
            hash_table_[i_index] = PolinomialHashTableSecondLevel(&gen, backet);
            ++i_index;
        }
    }
};
