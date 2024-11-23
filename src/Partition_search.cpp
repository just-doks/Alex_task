#include <iostream>
#include <cstdlib> // system()

#include "Bytes.h"
#include "Partition.h"

bool Partition::is_open() const
{
    if (m_pbr.is_fat() && m_drive.is_open())
        return true;
    return false;
}

void Partition::print_file_info(const FileInfo& file)
{
    std::cout << "Имя: ";
    if (file.name.length() == 0) 
        std::cout << "-\n";
    else 
        std::cout << file.name << '\n';
    std::cout << "Тип: ";
    switch(file.type)
    {
        case  DIR:      std::cout << "директория\n";          break;
        case FILE:      std::cout << "файл\n";                break;
        case ROOT_DIR:  std::cout << "корневая директория\n"; break;
        case NONE:      std::cout << "неизвестно\n";          return;
    }
    if (file.type == FILE)
        std::cout << "Размер файла: " << file.size << " байт\n";
    std::cout << "Номер первого кластера: " << file.first_cluster << '\n';
    std::cout << "Количество кластеров: "
              << count_file_clusters(file) << '\n';
    std::cout << "Степень фрагментации: ";
    if (uint32_t fragments = is_file_fragmented(file))
    {
        std::cout << fragments << " фрагментов\n";
    }
    else
        std::cout << " не фрагментирован\n";
}

void Partition::init(const std::string& path)
{
    std::string instruction = "umount ";
    instruction += path;
    system(instruction.c_str());
    m_drive.open(path, std::ios::binary | std::ios::in | std::ios::out);
    if (m_drive.is_open())
    {
        m_pbr.set(m_drive);
        m_FAT.resize(m_pbr.get_parameters().fat_size);
        m_drive.seekg(m_pbr.get_parameters().fat_offset, m_drive.beg);
        m_drive.read(m_FAT, m_pbr.get_parameters().fat_size);
    }
}

Partition::FileInfo Partition::get_root_dir()
{
    FileInfo file;
    file.name = "/";
    file.partition_sn = m_pbr.get_parameters().serial_number;
    file.type = ROOT_DIR;
    file.first_cluster = m_pbr.get_parameters().root_dir_cluster;
    file.size = m_pbr.get_parameters().root_dir_size;
    if (m_pbr.get_parameters().fat_type == PBR::FAT12 
        || m_pbr.get_parameters().fat_type == PBR::FAT16)
        file.entry_offset = m_pbr.get_parameters().data_offset;
    if (m_pbr.get_parameters().fat_type == PBR::FAT32)
        file.entry_offset = m_pbr.get_parameters().data_offset
            + m_pbr.get_parameters().cluster_size * (file.first_cluster - 1);
    return file;
}

Partition::FileInfo Partition::get_file(std::string& path)
{
    if (path == "/")
    {
        return get_root_dir();
    }
    std::string filename;

    FileInfo file {};
    
    uint32_t current_cluster = m_pbr.get_parameters().root_dir_cluster;
    uint32_t root_dir_size = m_pbr.get_parameters().root_dir_size;
    uint32_t dir_size = root_dir_size;
    uint32_t cluster_size = m_pbr.get_parameters().cluster_size;
    uint64_t data_offset = m_pbr.get_parameters().data_offset;
    uint64_t cluster_offset;
    uint32_t shift = 1U; // сдвиг из-за нулевого блока таблицы FAT

    Bytes dir_cluster(dir_size);

    cut_string(path, '/');
    
    while (path != "")
    {
        filename = extract_name(path);
        do
        {
            cluster_offset = data_offset
                + (current_cluster - shift) * cluster_size;
            
            m_drive.seekg(cluster_offset, m_drive.beg);
            m_drive.read(dir_cluster, dir_size);

            file = search_file(filename, dir_cluster);

            if (dir_size != cluster_size)
            {
                data_offset += root_dir_size;
                dir_size = cluster_size;
                shift = 2U; // прибавляем кластер корневой директории
                dir_cluster.resize(dir_size);
            }
            if (file.type != NONE)
            {
                file.entry_offset += cluster_offset;
                break;
            }
            current_cluster = m_FAT.get_value<uint32_t>
                (current_cluster * 2, Bytes::WORD);

        } while (current_cluster != 0xFFFF);
        
        cut_string(path, '/');

        if (file.type != DIR && path.length() != 0)
        {
            file = {};
            break;
        }
        if (file.type == DIR)
        {
            current_cluster = file.first_cluster;
        }
    }
    return ((file.type != NONE) ? file : FileInfo{});
}

Partition::FileInfo Partition::search_file(std::string& filename, Bytes& dir)
{
    FileInfo file {};
    for (size_t i = 0; i < dir.length(); i += 0x20)
    {
        char ch = dir.get_value<char>(i);
        if (static_cast<unsigned char>(ch) == 0xE5U 
            || ch == '.')
            continue;
        if (ch == 0)
            break;
        file.name = get_entry_name(dir, i);
        if (file.name == filename)
        {
            file.partition_sn = m_pbr.get_parameters().serial_number;
            file.type = get_file_type(dir, i);
            if (uint16_t f_c_h = dir.get_value<uint16_t>(i + 0x14))
            {
                file.first_cluster += static_cast<uint32_t>(f_c_h) << 16U;
            }
            file.first_cluster += dir.get_value<uint16_t>(i + 0x1A);
            file.size = dir.get_value<uint32_t>(i + 0x1C);
            file.entry_offset = i;
            break;
        }
    }
    return ((file.type != NONE) ? file : FileInfo{});
}

std::string Partition::get_entry_name(Bytes& dir, size_t offset)
{
    std::string name;
    char ch;
    for (uint8_t j = 0; j < 8; ++j)
    {
        ch = dir.get_value<char>(offset + j);
        //std::cout << ch;
        if (ch == 0x20) 
            break;
        name += ch;
    }
    ch = dir.get_value<char>(offset + 0xB);
    if ((ch & 0x10) == 0)
    {
        name += '.';
        for (uint8_t j = 0x8; j <= 0xA; ++j)
        {
            
            ch = dir.get_value<char>(offset + j);
            if (ch == 0x20) 
                break;
            name += ch;
        }
    }
    return name;
}

Partition::FileType Partition::get_file_type(Bytes& dir, size_t offset)
{
    FileType type = NONE;
    char ch = dir.get_value<char>(offset + 0xB);
    if (ch == 0x10)
        type = DIR;
    if (ch == 0x20)
        type = FILE;
    return type;
}

std::string Partition::extract_name(const std::string& path)
{
    std::string name;
    for (const char& el : path)
    {
        if (el == '/')
            break;
        name += el;
    }
    return name;
}

void Partition::cut_string(std::string& path, char ch)
{
    std::string new_path = "";
    char flag = 1;
    for (auto& el : path)
    {    
        if (flag) 
        {
            if (el != ch) 
                continue;
            else 
            {
                flag = 0;
                continue;
            }
        }
        new_path += el;
    }
    path = new_path;
}

Partition::FileInfo Partition::get_file_from_entry(Bytes& dir,
        uint32_t dir_cluster_number, size_t offset)
{
    FileInfo file = {};
    unsigned char ch = dir.get_value<unsigned char>(offset);
    if (ch == 0xE5U || ch == '.' || ch == 0)
        return file;
    file.type = get_file_type(dir, offset);
    if (file.type != NONE)
    {
        auto data_offset = m_pbr.get_parameters().data_offset;
        auto root_dir_size = m_pbr.get_parameters().root_dir_size;
        auto cluster_size = m_pbr.get_parameters().cluster_size;
        auto fat_type = m_pbr.get_parameters().fat_type;

        file.name = get_entry_name(dir, offset);
        file.partition_sn = m_pbr.get_parameters().serial_number;
        if (uint16_t f_c_h = dir.get_value<uint16_t>(offset + 0x14))
        {
            file.first_cluster += static_cast<uint32_t>(f_c_h) << 16U;
        }
        file.first_cluster += dir.get_value<uint16_t>(offset + 0x1A);
        file.size = dir.get_value<uint32_t>(offset + 0x1C);
        if (fat_type == PBR::FAT12 || fat_type == PBR::FAT16)
            file.entry_offset = data_offset + root_dir_size
                + (cluster_size * (dir_cluster_number - 2U)) + offset;
        if (fat_type == PBR::FAT32)
            file.entry_offset = data_offset
                + (cluster_size * (dir_cluster_number - 1U)) + offset;
    }
    return file;
}