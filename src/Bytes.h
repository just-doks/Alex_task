#pragma once

#include <cstdint>
#include <cassert>
#include <type_traits> // std::is_integral<T>::value

// Контейнерный класс для хранения байт. Используется, в основном,
// в качестве буфера. Позволяет быстро обработать данные, 
// при необходимости, изменить и записать обратно в файл.
class Bytes
{       
    protected:
        /// Символьный динамический массив для хранения байт.
        char* m_bytes;
        // Переменная, хранящая размер массива.
        size_t m_size;

        // Скрытый метод копирования для реализации глубокого
        // копирования (чтобы избежать копирования адресов).
        Bytes& copy(const Bytes& b);

    public:
        Bytes() : m_size(0)
            {   m_bytes = nullptr;   }
        Bytes(size_t size) : m_size(size)
            {   m_bytes = new char[size];   }
        Bytes(const Bytes& b)
            {   copy(b);   }

        // Перегрузка данных операторов позволяет при передаче
        // экземпляра класса передавать указатель на символьный массив,
        // когда это ожидается.
        operator char*() { return m_bytes; }
        operator const char*() const { return m_bytes; }

        // Перегруженный оператор= для присваивания экземпляров класса
        // для реализации глубокого копирования (выделяется новая память
        // и элементы копируются по значению).
        Bytes& operator=(const Bytes& b) { return copy(b); }

        // Перегруженный оператор[] для получения элементов массива по индексу.
        char& operator[](size_t index) 
        {
            assert(index < m_size && "index is out of range."); 
            return m_bytes[index]; 
        }

        // Метод возвращает длину символьного массива.
        auto length() const -> size_t { return m_size; }

        // Возвращает указатель на символьный массив.
        auto get_pointer() -> char* { return m_bytes; }

        // Очищение (освобождение) выделенной памяти. Обнуление размера.
        auto clear() -> Bytes&;

        // Изменяет размер выделенной памяти для буфера.
        auto resize(size_t length) -> Bytes&;

        // Возвращает строку из байт указанного размера, по указанному смещению.
        auto get_string(size_t offset, size_t size) const -> std::string;
        
        // Функции вывода байт в контейнере. В работе программы не используются.
        // Нужны при проверке работы кода.
        auto print_bytes() const -> void;
        static void print_bytes(char* buff, size_t amount);

        // Вложенный тип (перечисление). Используется для указания 
        // размера байт, которое необходимо считать или записать в буфер. 
        enum TypeSize
        {
            BYTE = 1,
            WORD = 2,
            DOUBLE_WORD = 4
        };

        // Следующие два шаблона метода возвращают значения из указанных байт.
        // Они могут возвращать результат разных типов и размеров. 
        // Из-за особенности хранения байт данных в файловой системе FAT 
        // (обратная последовательность байт), данные функции также решают
        // и эту проблему, возвращая корректный результат.
        template <typename T>
        T get_value(size_t offset) const;

        template <typename T>
        T get_value(size_t offset, TypeSize size) const;

        // Следующие два шаблона методов вставляют указанные значения,
        // которые могут быть разных размеров и разных типов,
        // в символьный (байтовый) буфер. Впоследствие, буфер может быть
        // использован для записи его в файл.
        template <typename T>
        void insert(T value, size_t offset, TypeSize size);

        template <typename T>
        void insert(T value, size_t offset);

        // Деструктор класса. При выходе экземпляра из области видимости,
        // выделенная память под символьный массив освобождается.
        ~Bytes()
        {
            if (m_bytes != nullptr)
                delete[] m_bytes;
        }
};

// Реализация шаблонов обязана быть в одном файле с их объявлением.
template <typename T>
T Bytes::get_value(size_t offset) const
{
    static_assert(std::is_integral<T>::value, "Expected integral type.");
    assert(offset + sizeof(T) <= m_size && "Invalid offset.");
    T value = static_cast<T>(0);
    for (int i = sizeof(T) - 1; i >= 0; --i)
        value |= static_cast<T>(static_cast<unsigned char>(m_bytes[offset + i])) << 8 * i;
    return value;
}

template <typename T>
T Bytes::get_value(size_t offset, TypeSize size) const
{
    static_assert(std::is_integral<T>::value, "Expected integral type.");
    assert(size <= sizeof(T)  && "Size is bigger than type size.");
    assert(offset + size <= m_size && "Invalid offset and size.");
    T value = static_cast<T>(0);
    for (int i = size - 1; i >= 0; --i)
        value |= static_cast<T>(static_cast<unsigned char>(m_bytes[offset + i])) << 8 * i;
    return value;
}


template <typename T>
void Bytes::insert(T value, size_t offset, TypeSize size)
{
    static_assert(std::is_integral<T>::value, "Expected integral type.");
    assert(size <= sizeof(T)  && "Size is bigger than type size.");
    assert(offset + size <= m_size && "Invalid offset and size.");
    for (size_t i = 0; i < size; ++i)
    {
        m_bytes[offset + i] = static_cast<char>(value >> 8 * i);
    }
}

template <typename T>
void Bytes::insert(T value, size_t offset)
{
    static_assert(std::is_integral<T>::value, "Expected integral type.");
    //assert(size <= sizeof(T)  && "Size is bigger than type size.");
    assert(offset + sizeof(T) <= m_size && "Invalid offset and size.");
    for (size_t i = 0; i < sizeof(T); ++i)
    {
        m_bytes[offset + i] = static_cast<char>(value >> 8 * i);
    }
}
