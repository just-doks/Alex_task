#include <iostream>
#include <cstdint>
#include <cassert>
#include <type_traits> // std::is_integral<T>::value

#include "Bytes.h"

Bytes& Bytes::copy(const Bytes& b)
{
    if (&b == this)
        return *this;
    delete[] m_bytes;
    if (b == nullptr)
        m_size = 0;
    else
        m_size = b.length();
    for (size_t i = 0; i < m_size; ++i)
    {
        *(m_bytes + i) = *(b + i);
    }
    return *this;
}

auto Bytes::clear() -> Bytes&
{
    if (m_bytes != nullptr)
    {
        delete[] m_bytes;
        m_bytes = nullptr;
    }
    m_size = 0;
    return *this;
}

auto Bytes::resize(size_t length) -> Bytes&
{
    if (length == 0)
        return clear();
    if (length == m_size)
        return *this;
    
    if (m_bytes == nullptr)
    {
        m_bytes = new char[length];
        m_size = length;
    }
    else
    {
        delete[] m_bytes;
        m_bytes = new char[length];
        m_size = length;
    }
    return *this;
} 

auto Bytes::get_string(size_t offset, size_t size) const -> std::string
{
    assert(offset + size < m_size && "Invalid offset and size.");
    char* buff = new char[size + 1];
    for (int i = 0; i < size; ++i)
        buff[i] = m_bytes[offset + i];
    buff[size] = '\0';
    std::string str = buff;
    delete[] buff;
    return str;
} 

void Bytes::print_bytes() const
{
    unsigned char* bytes = reinterpret_cast<unsigned char*>(m_bytes);
    std::cout << std::hex << std::uppercase;
    int counter = m_size;
    for (int i = 0; i < ((m_size > 16) ? (m_size / 16) : 1); ++i)
    {
        std::cout << i + 1 << '\t';
        for (int j = 0; j < 16; ++j)
        {
            if (counter == 0)
                break;
            else if (*(bytes + i*16 + j) > 0xF)
                std::cout << static_cast<uint16_t>(*(bytes + i*16 + j));
            else
                std::cout << '0' << static_cast<uint16_t>(*(bytes + i*16 + j));
            
            if (j == 7)
                std::cout << "   ";
            std::cout << ' ';
            if (counter > 0) --counter;
        }
        std::cout << '\n';
        if (((i + 1) % 16) == 0) 
            std::cout << '\n'; 
    }

    std::cout << std::dec;
}

void Bytes::print_bytes(char* buff, size_t amount) 
{
    unsigned char* bytes = reinterpret_cast<unsigned char*>(buff);
    std::cout << std::hex << std::uppercase;
    for (size_t i = 0; i < amount; ++i)
    {
        if ((i & 0xFU) == 0)
            std::cout << i << '\t';
        if (bytes[i] > 0xFU)
            std::cout << static_cast<uint16_t>(bytes[i]);
        else
            std::cout << '0' << static_cast<uint16_t>(bytes[i]);

        if ((i & 0xFU) == 7U)
        {
            std::cout << "    ";
            continue;
        }
        if ((i & 0xFFU) == 0xFFU)
            std::cout << '\n';
        if ((i & 0xFU) == 0xFU) 
            std::cout << '\n';
        else
            std::cout << " ";
    }

    std::cout << std::dec;
}