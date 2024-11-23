#ifndef PBR_H
#define PBR_H

#include <fstream>
#include <cstdint>

#include "Bytes.h"

/* Класс, используемый для получения и обработки данных о разделах.
 * Считывается первый сектор раздела и определяется, относится ли
 * раздел к искомому. При положительном результате, извлекаются
 *  полезные данные. */
class PBR // Partition Boot Record (Загрузочная запись раздела).
{
    public:
        // Перечисление для определения и сверки типа раздела.
        enum FAT_Type
        {
            FAT12,
            FAT16, // Данная программа реализует поддержку только FAT16
            FAT32,
            NONE
        };
        // Структура для хранения исчерпывающей информации о разделе.
        struct Parameters
        {
            uint32_t cluster_size = 0;
            uint64_t fat_offset = 0;
            uint8_t fat_number = 0;
            uint64_t fat_size = 0;
            uint64_t data_offset = 0;
            uint64_t partition_size = 0;
            uint32_t root_dir_size = 0;
            uint32_t clusters_number = 0;
            FAT_Type fat_type = NONE;
            uint32_t root_dir_cluster = 0;
            uint32_t last_cluster = 0;
            uint32_t serial_number = 0;
            std::string label;
        };

    private:
        // Контейнер для хранения байт записи раздела.
        Bytes m_buff;
        // Экземпляр структуры.
        Parameters m_parameters;

        // Методы для инициализации данных экземпляра класса.
        // Они открывают специальный файл устройства (раздела)
        // и переносят загрузочную запись в контейнер.
        void init(const std::string& path, uint64_t offset = 0);
        void init(std::fstream& drive);

        // Главный метод класса - он инициализирует структуру
        // данными, извлекаемыми из байт загрузочной записи раздела.
        void set_pbr(uint64_t offset = 0);

    public:
        // Конструкторы класса.
        PBR();
        PBR(const std::string path, uint64_t offset = 0);

        // Проверка сигнатуры загрузочной записи. Проверяется сигнатура
        // загрузочной записи, а также сигнатура записи FAT раздела.
        bool is_pbr() const;

        // Если экземпляр класса считал истинный раздел FAT, метод
        // возвращает true, иначе false.
        bool is_fat() const;

        // Два метода для считывания указанного раздела и перезаписи
        // информации в экземпляре. 
        auto set(const std::string& path, uint64_t offset = 0) -> void;
        auto set(std::fstream& drive) -> void;

        // Метод для очистки (обнуления) данных экземпляра. Используется
        // перед переинициализацией методами set(). 
        auto clear() -> PBR&;
        
        // Самый часто используемый метод - возвращает ссылку на структуру
        // с данными по разделу. Позволяет обращаться к данным напрямую,
        // а изменение данных извне запрещено.
        auto get_parameters() const -> const Parameters&
         { return m_parameters; }

        // Метод возвращает ссылку на контейнер. Не востребован.
        Bytes& get_bytes() { return m_buff; }

        // Вывод значений структуры. Используется при отладке и тестах.
        void print_parameters() const;

        // Используется для оповещения пользователя об обнаруженном разделе.
        // Выводит основную, полезную пользователю информацию.
        void print() const;

};

#endif // PBR_H