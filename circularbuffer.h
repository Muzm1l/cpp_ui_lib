#ifndef CIRCULARBUFFER_H
#define CIRCULARBUFFER_H

#include <vector>
#include <cstddef>
#include <algorithm>
#include <stdexcept>

/**
 * @brief A circular buffer implementation that maintains a fixed maximum size
 * 
 * When the buffer reaches capacity, new elements overwrite the oldest elements (FIFO).
 * This prevents unbounded memory growth while maintaining recent data.
 * 
 * @tparam T The type of elements stored in the buffer
 */
template<typename T>
class CircularBuffer
{
public:
    /**
     * @brief Construct a circular buffer with specified capacity
     * @param capacity Maximum number of elements (0 = unlimited, not recommended)
     */
    explicit CircularBuffer(size_t capacity = 0) : m_capacity(capacity), m_startIndex(0), m_size(0)
    {
        if (capacity > 0)
        {
            m_data.reserve(capacity);
        }
    }
    
    /**
     * @brief Get the current number of elements in the buffer
     */
    size_t size() const { return m_size; }
    
    /**
     * @brief Get the maximum capacity of the buffer
     */
    size_t capacity() const { return m_capacity; }
    
    /**
     * @brief Check if the buffer is empty
     */
    bool empty() const { return m_size == 0; }
    
    /**
     * @brief Check if the buffer is full (size == capacity)
     */
    bool full() const { return m_capacity > 0 && m_size >= m_capacity; }
    
    /**
     * @brief Set the capacity of the buffer
     * If new capacity is smaller than current size, oldest elements are removed
     */
    void setCapacity(size_t capacity)
    {
        if (capacity == 0)
        {
            // Unlimited capacity - just clear the circular buffer logic
            m_capacity = 0;
            return;
        }
        
        if (capacity < m_size)
        {
            // Need to remove oldest elements
            size_t elementsToRemove = m_size - capacity;
            m_startIndex = (m_startIndex + elementsToRemove) % (m_data.empty() ? 1 : m_data.size());
            m_size = capacity;
        }
        
        m_capacity = capacity;
        
        // If we're using circular buffer mode and need to resize underlying vector
        if (m_capacity > 0 && m_data.size() < m_capacity)
        {
            m_data.reserve(m_capacity);
        }
    }
    
    /**
     * @brief Add an element to the buffer (overwrites oldest if full)
     */
    void push_back(const T& value)
    {
        if (m_capacity == 0)
        {
            // Unlimited capacity - just append
            m_data.push_back(value);
            m_size++;
            return;
        }
        
        if (m_size < m_capacity)
        {
            // Buffer not full yet - just append
            if (m_data.size() < m_capacity)
            {
                m_data.push_back(value);
            }
            else
            {
                // Vector is at capacity, but we haven't wrapped yet
                size_t actualIndex = (m_startIndex + m_size) % m_capacity;
                m_data[actualIndex] = value;
            }
            m_size++;
        }
        else
        {
            // Buffer is full - overwrite oldest element
            m_data[m_startIndex] = value;
            m_startIndex = (m_startIndex + 1) % m_capacity;
        }
    }
    
    /**
     * @brief Add multiple elements to the buffer
     */
    void push_back(const std::vector<T>& values)
    {
        for (const T& value : values)
        {
            push_back(value);
        }
    }
    
    /**
     * @brief Insert elements at the end (same as push_back for circular buffer)
     */
    void insert(typename std::vector<T>::const_iterator begin, typename std::vector<T>::const_iterator end)
    {
        for (auto it = begin; it != end; ++it)
        {
            push_back(*it);
        }
    }
    
    /**
     * @brief Get element at index (0 = oldest, size-1 = newest)
     */
    const T& operator[](size_t index) const
    {
        if (m_capacity == 0)
        {
            return m_data[index];
        }
        
        size_t actualIndex = (m_startIndex + index) % m_capacity;
        return m_data[actualIndex];
    }
    
    /**
     * @brief Get element at index (non-const version)
     */
    T& operator[](size_t index)
    {
        if (m_capacity == 0)
        {
            return m_data[index];
        }
        
        size_t actualIndex = (m_startIndex + index) % m_capacity;
        return m_data[actualIndex];
    }
    
    /**
     * @brief Get the oldest element
     */
    const T& front() const
    {
        if (m_size == 0)
        {
            throw std::out_of_range("CircularBuffer is empty");
        }
        return (*this)[0];
    }
    
    /**
     * @brief Get the newest element
     */
    const T& back() const
    {
        if (m_size == 0)
        {
            throw std::out_of_range("CircularBuffer is empty");
        }
        return (*this)[m_size - 1];
    }
    
    /**
     * @brief Clear all elements from the buffer
     */
    void clear()
    {
        m_data.clear();
        m_startIndex = 0;
        m_size = 0;
    }
    
    /**
     * @brief Reserve capacity in underlying vector (for performance)
     */
    void reserve(size_t capacity)
    {
        if (m_capacity == 0 || capacity <= m_capacity)
        {
            m_data.reserve(capacity);
        }
        else
        {
            m_data.reserve(m_capacity);
        }
    }
    
    /**
     * @brief Get all elements as a vector (in chronological order, oldest first)
     */
    std::vector<T> toVector() const
    {
        std::vector<T> result;
        result.reserve(m_size);
        
        for (size_t i = 0; i < m_size; ++i)
        {
            result.push_back((*this)[i]);
        }
        
        return result;
    }
    
    /**
     * @brief Replace all elements with new data
     */
    void assign(const std::vector<T>& newData)
    {
        clear();
        if (m_capacity == 0)
        {
            m_data = newData;
            m_size = newData.size();
        }
        else
        {
            // Add elements one by one to respect circular buffer behavior
            for (const T& value : newData)
            {
                push_back(value);
            }
        }
    }
    
    /**
     * @brief Erase a series label (for map-based usage)
     */
    void erase() { clear(); }
    
    /**
     * @brief Erase elements that match a predicate
     * @param pred Predicate function that returns true for elements to erase
     * @return Number of elements erased
     */
    template<typename Predicate>
    size_t erase_if(Predicate pred)
    {
        if (m_capacity == 0)
        {
            // For unlimited capacity, use standard vector erase
            size_t oldSize = m_data.size();
            m_data.erase(std::remove_if(m_data.begin(), m_data.end(), pred), m_data.end());
            m_size = m_data.size();
            return oldSize - m_size;
        }
        
        // For circular buffer, convert to vector, erase, then rebuild
        std::vector<T> temp = toVector();
        size_t oldSize = temp.size();
        temp.erase(std::remove_if(temp.begin(), temp.end(), pred), temp.end());
        clear();
        for (const T& value : temp)
        {
            push_back(value);
        }
        return oldSize - m_size;
    }
    
    /**
     * @brief Get reference to underlying vector (for compatibility with existing code)
     * Note: This returns the actual storage, which may be in circular order
     * Use toVector() or operator[] for chronological access
     */
    const std::vector<T>& getData() const { return m_data; }
    std::vector<T>& getData() { return m_data; }

private:
    std::vector<T> m_data;      // Underlying storage
    size_t m_capacity;          // Maximum capacity (0 = unlimited)
    size_t m_startIndex;        // Index of oldest element (for circular buffer mode)
    size_t m_size;              // Current number of elements
};

#endif // CIRCULARBUFFER_H

