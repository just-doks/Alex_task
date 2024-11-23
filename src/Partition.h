#ifndef PARTITION_H
#define PARTITION_H

#include <cstdint> // uint_t
#include <fstream> // std::fstream
#include "PBR.h"
#include "Bytes.h"

// Класс, отвечающий за взаимодействие с разделом.
// Функции поиска файла и дефрагментации лежат в его реализации.
class Partition
{
    public:
        // Перечисление для определения и сравнения типа файла.
        enum FileType
        {
            NONE = 0,
            DIR,
            FILE,
            ROOT_DIR
        };

        // Внутренний класс, экземпляры которого используются
        // для хранения сведений о файлах. В отличие от структур,
        // позволяет скрыть поля и сделать внешние вмешательства
        // недопустимыми, однако публичность класса позволяет 
        // получать из класса Partition результат в виде экземпляра
        // класса FileInfo, который можно сохранить и впоследствии
        // передать обратно в класс Partition для обработки.
        class FileInfo
        {
            friend class Partition;
            private:
            uint32_t partition_sn = 0;
            FileType type = NONE;
            uint32_t first_cluster = 0;
            uint32_t size = 0;
            uint64_t entry_offset = 0;
            std::string name;
            public: FileInfo() {}
            const std::string& get_name() const
            { return name; }
            const FileType get_type() const
            { return type; }
        };
        // Функция вывода информации об обнаруженном файле.
        void print_file_info(const FileInfo&);
    protected:
        // Экземпляр класса PBR(Partition Boot Record).
        // Используется для хранения сведений о разделе.
        // К нему регулярно приходится обращаться для работы с разделом.
        PBR m_pbr;

        // Контейнер для байт таблицы FAT. Из него считываются данные
        // о занимаемых файлами кластерах. При необходимости, в таблицу
        // вносятся изменения, после чего, она может быть записана
        // обратно в файл устройства для фиксации изменений.
        Bytes m_FAT;

        // Экземпляр, реализующий доступ к файлу устройства.
        // Через него осуществляется доступ к файлам в разделе, его данным.
        // С помощью этого экземпляра также осуществляется запись
        // всех изменений на раздел.
        std::fstream m_drive;

        // Метод для инициализации экземпляра класса:
        // - Открывается поток к файлу устройства для чтения и записи;
        // - Считывается загрузочная запись раздела, и если запись подлинная,
        // - Считывается таблица FAT в контейнер.
        auto init(const std::string& path) -> void;

        // Поиск файла:

        // Вспомогательные методы для поиска файла.
        // Метод извлекает имя файла/директории до первого слэша ("/").
        auto extract_name(const std::string& path) -> std::string;
        // Метод обрезает строку, в которой указан путь к файлу, от начала
        // до первого слэша ("/").
        auto cut_string(std::string& path, char ch) -> void;

        // Метод ищет файл по указанному имени в кластере буферизованной
        // директории. При нахождении, возвращает заполненный экземпляр
        // класса FileInfo с данными файла.
        auto search_file(std::string& filename, 
            Bytes& dir_cluster) -> Partition::FileInfo;
        // Вспомогательный метод для извлечения имени файла из байт
        // в буфферизованной директории. Имя затем сравнивается с искомым.
        auto get_entry_name(Bytes& dir, size_t offset) -> std::string;
        // Вспомогательный метод, извлекающий из байт буфферизованной 
        // директории информацию о типе файла (файл, директория, неизвестное).
        auto get_file_type(Bytes& dir, size_t offset) -> FileType;
        
        // Метод для получения экземпляра класса из буфферизованной
        // директории по указанному смещению байт. Номер кластера
        // директории указывается, чтобы заполнить информацию
        // о смещении записи файла в разделе, для возможного внесения
        // изменений.
        auto get_file_from_entry(Bytes& dir,
            uint32_t dir_cluster_number, size_t offset) -> FileInfo;

        // Метод возвращает экземпляр, заполненный данными по корневой
        // директории. 
        auto get_root_dir() -> FileInfo;

        // Обнаружение/устранение фрагментации

        // Главная функция дефрагментации - осуществляет проверку
        // на фрагментацию и дефрагментацию указанного файла или каталога, 
        // поскольку, каталоги также, в теории, могут быть фрагментированы. 
        auto defragment_file(FileInfo& file) -> uint32_t;
        // Вспомогательный метод при обработке директорий.
        // Проходится по всем кластерам директории, вызывая
        // для каждого кластера метод defragment_dir_cluster().
        auto defragment_dir(FileInfo& file) -> uint32_t;
        // Метод, извлекающий из буферизованных кластеров директорий
        // файлы, которые затем передаёт в метод defragment_file().
        auto defragment_dir_cluster(Bytes& dir_cluster, 
            uint32_t cluster_number) -> uint32_t;
        // Метод, копирующий указанный кластер по указанному адресу.
        auto copy_cluster(uint32_t source, uint32_t destination) -> void;
        // Метод, используемый для поиска требуемого свободного пространства
        // для дефрагментации файла.
        auto find_empty_space(uint32_t clusters_number) -> uint32_t;
        // Метод для подсчёта занимаемых файлом кластеров.
        // Универсален для любых типов файлов, поскольку высчитывает
        // кластеры по таблице FAT из контейнера.
        auto count_file_clusters(const FileInfo& file) -> uint32_t;

    public:
        // Конструктор класса. Позволяет создать экземпляр только
        // при указании пути. При некорректном пути, потребуется
        // создавать новый экземпляр. Это сказывается на гибкости,
        // но несколько сокращает возможные ошибки.
        Partition(const std::string& path) { init(path); }

        // Метод для проверки, был ли инициализирован экземпляр корректно.
        auto is_open() const -> bool;

        // Метод для получения экземпляра класса FileInfo. 
        // Используется для поиска файла по заданному пути для дальнейших
        // манипуляций, а именно, дефрагментации.
        auto get_file(std::string& path) -> FileInfo;

        // Метод, используемый для проверки файла на фрагментацию.
        auto is_file_fragmented(const FileInfo& file) -> uint32_t;
        // Открытый метод, запускающий процесс дефрагментации файла.
        // Возвращает количество дефрагментированных файлов.
        auto defragment(FileInfo& file) -> uint32_t;

        ~Partition() 
        {
            if (m_drive.is_open())
                m_drive.close();
        }
};

#endif // PARTITION_H