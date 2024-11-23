#include "System_launch.h"

#include "Program.h"

#include <iostream> // std::cout, std::cin, std::streamsize
#include <cstdlib> // system()
#include <limits> // std::numeric_limits<>::max()
#include <filesystem> // directory_iterator()
#include <cstring> // strcat, strcpy

#include "PBR.h"
#include "Partition.h"

void Program::start()
{
    int ch = -1;
    do
    {
        system("clear");
        std::cout << "Запустить поиск FAT разделов(1) "
            << "или открыть файл устройства(2)? (Выход - 0)\nОтвет: ";
        std::cin >> ch;
        if (std::cin.fail())
        {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            ch = -1;
        }
    } while (ch != 2 && ch != 1 && ch != 0);

    std::cout << '\n';

    std::string path;
    switch(ch)
    {
        case 1:
            {
                std::string dev = "/dev/";
                path = fat_search(dev);
                break;
            }
        case 2:
            {
                std::cout << "Укажите путь до файла устройства.\n"
                    << "Путь: ";
                std::cin >> path;
                break;
            }
        case 0:
            return;
    }

    if (path.length())
    {
        if (ch == 1)
            std::cout << "Специальный файл устройства: " << path << '\n';
        do
        {
            open_partition(path);
            std:: cout << "Повторить поиск?\n(1 - да, любая клавиша - нет): ";
            std::cin >> ch;
            // Проверка ввода
            if (std::cin.fail())
            {
                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                ch = -1;
                continue;
            }
        } while (ch == 1);   
    }
    else
    {
        std::cout << "Разделы с файловой системой FAT не найдены.\n";
    }
}

std::string Program::fat_search(const std::string& directory)
{
    // sd[a-z][optional: a-z][1-15]
    std::vector<std::string> sd_list = get_files_from_dir
        ( directory, is_partition );

    std::vector<std::string> fat_list = find_fat_partitions(sd_list);

    if (fat_list.size())
    {
        int num = -1;
        do
        {
            std::cout << "Укажите номер раздела(выйти - 0): ";
            std::cin >> num;

            // Проверка ввода
            if (std::cin.fail())
            {
                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                num = -1;
                continue;
            }

        } while (!(num >= 0 && num <= fat_list.size()));

        std::cout << '\n';

        if (num > 0)
            return fat_list[num - 1];
    }
    return std::string();
}

void Program::open_partition(const std::string& sd_filename)
{
    
    Partition partition(sd_filename);
    if (!partition.is_open())
    {
        std::cout << "Некорректный путь или файл устройства.\n";
        return;
    }
    int num = -1;
    std::string path;
    // Запрашиваем имя файла
    std::cout << "\nУкажите путь до файла или каталога в формате:\n"
        << "/PATH/.../NAME\nПуть: ";
    std::cin >> path;
    std::cout << '\n';

    Partition::FileInfo file;
    file = partition.get_file(path);
    if (file.get_type() == Partition::NONE)
        std::cout << "Файл по указанному пути не найден.\n";
    else
    {
        system("clear");
        partition.print_file_info(file);
        fragmentation_check(partition, file);
    }
}

void Program::fragmentation_check(Partition& p, Partition::FileInfo& file)
{
    if (  (file.get_type() == Partition::FILE && p.is_file_fragmented(file))
        || file.get_type() == Partition::DIR 
        || file.get_type() == Partition::ROOT_DIR)
    {
        int num = -1;
        if (file.get_type() == Partition::FILE)
            std::cout << "Запустить дефрагментацию?\n";
        else
            std::cout << "Запустить дефрагментацию вложенных файлов?\n";
        do
        {
            std::cout << "(1 - да, 0 - нет): ";
            std::cin >> num;

            // Проверка ввода
            if (std::cin.fail())
            {
                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                num = -1;
                continue;
            }
        } while (num != 0 && num != 1);
        
        if (num == 1)
        {
            int amount = p.defragment(file);
            std::cout << "Было фрагментировано: " << amount << " файлов.\n";
        }
    }
    
}

#ifdef LINUX
bool Program::is_partition(const std::string& str)
{
    if (str[0] != 's'  && str[1] != 'd')
        return false;
    if (str.size() == 4)
        if (    str[2] >= 'a' && str[2] <= 'z' 
            &&  str[3] >= '1' && str[3] <= '9') // sd[a-z][1-9]
            return true;
    if (str.size() == 5)
    {
        if (    str[2] >= 'a' && str[2] <= 'z' 
            &&  str[3] >= 'a' && str[3] <= 'z'
            &&  str[4] >= '1' && str[4] <= '9') // sd[a-z][a-z][1-9]
            return true;
        else if ( str[2] >= 'a' && str[2] <= 'z' 
            &&    str[3] == '1'
            &&    str[4] >= '0' && str[4] <= '5') // sd[a-z][10-15]
            return true;
    }
    if (str.size() == 6)
        if (    str[2] >= 'a' && str[2] <= 'z' 
            &&  str[3] >= 'a' && str[3] <= 'z'
            &&  str[3] == '1'
            &&  str[4] >= '0' && str[4] <= '5') // sd[a-z][a-z][10-15]
            return true;
    return false;
}
#endif // LINUX
#ifdef MAC_OS
bool Program::is_partition(const std::string& s)
{
    std::cout << s << '\n';
    if (s.size() < 7)
        return false;
    if (s[0] != 'd'  && s[1] != 'i' && s[2] != 's' && s[3] != 'k')
        return false;
    if (s.size() == 7)
        if (    s[4] >= '1' && s[4] <= '9' && s[5] == 's'
            &&  s[6] >= '1' && s[6] <= '9') // disk[1-9]s[1-9]
            return true;
    if (s.size() == 8)
    {
        if (    s[4] >= '1' && s[4] <= '9' && s[5] == 's'
            &&  s[6] == '1' && s[7] >= '0' && s[7] <= '5') // disk[1-9]s[10-15]
            return true;
        else if ( s[4] == '1' && s[5] >= '0' && s[5] <= '5'
            &&    s[6] == 's' && s[7] >= '0' && s[7] <= '9') // disk[10-15]s[0-9]
            return true;
    }
    if (s.size() == 9)
        if (    s[4] == '1' && s[5] >= '0' && s[5] <= '5' && s[6] == 's'
            &&  s[7] == '1' && s[8] >= '0' && s[8] <= '5') // disk[10-15]s[10-15]
            return true;
    return false;
}
#endif


/* Описание:
 * - Функция, проверящая в директории имена файлов и возвращающая список подходящих.
 * Параметры: 
 * - путь к директории;
 * - булевая функция, проверяющая строку.
 * Возвращаемое значение:
 * - вектор значений типа std::string (возврат по значению).
 * Дополнительно:
 * - Возврат по значению необходим, поскольку при возврате по адресу/ссылке
 *   будет возвращён висячий указатель, что является ошибкой. Это происходит, 
 *   поскольку при выходе точки выполнения программы из области видимости вектора, 
 *   вектор автоматически освобождает выделенную ему память.
 *   Чтобы это избежать, используется механизм элизии копирования - при возврате 
 *   по значению, в действительности, копирования не происходит. Локальный вектор 
 *   функции становится вектором caller-а (функции, вызвавшей данную функцию).
 *   Однако, механизм должен поддерживать компилятор. 
 *   clang++ и g++ поддерживают copy elision. */
std::vector<std::string> Program::get_files_from_dir
    ( 
        const std::string& directory_path,
        std::function<bool(const std::string&)> condition
    )
{
    std::vector<std::string> list;

    // Проверяем каждый файл директории:
    // для этого служит итератор directory_iterator() - в кач-ве аргумента передаётся
    // путь к директории, после чего, итератор можно использовать с помощью цикла foreach
    // для перебора всех значений (файлов директории).
    for (const auto& entry : std::filesystem::directory_iterator(directory_path))
    {
        // Если переданная функция, при получении имени файла из директории, возвращает 
        // значение true - добавляем полный путь к файлу в вектор, как строку типа std::string.
        if (condition( entry.path().filename().string() ))
            list.push_back(entry.path().string());
    }
    return list;    
}

std::vector<std::string> Program::find_fat_partitions
    ( const std::vector<std::string>& list )
{
    std::vector<std::string> fp_list;
    PBR pbr;
    int counter = 0;
    for (auto& el : list)
    {
        pbr.set(el);
        if (pbr.is_fat())
        {
            std::cout << "Номер раздела: " << ++counter << '\n';
            pbr.print();
            fp_list.push_back(el);
        }
        
    }
    return fp_list;
}