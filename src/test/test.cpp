#include <iostream>
#include "Partition.h"

// Наследник класса Partition, созданный с целью использования
// методов предшествующего класса в альтернативных сценариях.
// А именно, можно быстро вывести основную информацию о разделе,
// и скопировать указанный кластер по указанному адресу.
class TPart: public Partition
{
    public:
    TPart(const std::string& path) : Partition(path) {}

    void copy_cluster(uint32_t from, uint32_t to)
    {
        Partition::copy_cluster(from, to);
    }
    void print_pbr()
    {
        std::cout << std::hex << std::uppercase << '\n';
        m_pbr.print_parameters();
        std::cout << std::dec << '\n';
        m_pbr.print_parameters();
        std::cout << std::endl;
    }
    uint32_t get_cluster_size()
    {
        return m_pbr.get_parameters().cluster_size;
    }
};

// Класс Test аналогичен по назначению классу Program, 
// с той разницей, что предоставляет опции для тестирования
// и создания фрагментации, вместо её обнаружения и устранения.
class Test
{
    public:
        Test() {}

        // Метод создаёт файл указанного размера в кластерах,
        // размер которых также указывается. 
        // Файл заполняется однобайтовым символом.
        void create_file(std::string name, uint32_t size_in_clusters, uint32_t cl_s)
        {
            std::ofstream outfile(name);
            uint32_t limit = size_in_clusters * cl_s;
            for (uint32_t i = 0U; i < limit; ++i)
            {
                if ((i & 0x3FU) == 0x3FU)
                    outfile << '\n';
                else
                    outfile << '8';
            }
            outfile.close();
        }

        // Метод начала работы с экземпляром.
        // Начинает диалог с пользователем.
        // Предлагает открыть раздел или создать файл.
        void start()
        {
            int num = -1;
            std::cout   << "open_partition()\t- 1\n"
                        << "create_file()\t\t- 2\n";
            std::cout << "Answer: ";
            std::cin >> num;
            switch(num)
            {
                case 1:
                {
                    open_partition();
                    break;
                }
                case 2:
                {
                    std::string name;
                    uint32_t cl;
                    uint32_t cl_size;
                    std::cout << "Enter name, cl and cl_size: ";
                    std::cin >> name;
                    std::cin >> cl;
                    std::cin >> cl_size;
                    create_file(name, cl, cl_size);
                    break;
                }
            }
        }

        // Промежуточный метод, создающий экземпляр тестового класса.
        // Передаёт его в следующий метод.
        void open_partition()
        {
            std::cout << "Path: ";
            std::string path;
            std::cin >> path;
            TPart p(path);
            if (p.is_open())
            {
                p_options(p);
            }
        }

        // Данный метод продолжает диалог с пользователем, предлагая
        // скопировать выбранный кластер в указанную позицию,
        // либо напечатать подробную информацию о разделе.
        void p_options(TPart& p)
        {
            int num = -1;
            std::cout   << "copy_cluster()\t- 1\n"
                        << "print_pbr()\t- 2\n";
            std::cout << "Answer: ";
            std::cin >> num;
            switch(num)
            {
                case 1: 
                {
                    uint32_t src;
                    uint32_t dest;
                    char ch;
                    do
                    {
                        std::cout << "Source cluster: ";
                        std::cin >> src;
                        std::cout << "Destination cluster: ";
                        std::cin >> dest;
                        p.copy_cluster(src, dest);
                        std::cout << "Repeat?\n(y/n): ";
                        std::cin >> ch;
                    } while (ch == 'y');
                    break;
                }
                case 2:
                {
                    p.print_pbr();
                    break;
                }
            }
        }
};

int main(int argc, char* argv[])
{
    std::cout << "Start test.\n";
    Test test;
    test.start();
    
    return 0;
}