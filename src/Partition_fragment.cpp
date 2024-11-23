
#include "Partition.h"
#include "PBR.h"

#include <iostream>
#include <cassert>

uint32_t Partition::is_file_fragmented(const FileInfo& file)
{
    uint32_t fragments = 0;
    uint32_t current_cluster = file.first_cluster;
    uint32_t previous_cluster;
    do
    {
        previous_cluster = current_cluster;
        current_cluster = m_FAT.get_value<uint32_t>
            ( current_cluster * 2U, Bytes::WORD );
        if ((current_cluster != 0xFFFFU)
            && (current_cluster - previous_cluster) != 1)
        {
            ++fragments;
        }

    } while (current_cluster != 0xFFFFU);

    return ((fragments > 0) ? ++fragments : 0);
}

uint32_t Partition::defragment(FileInfo& file)
{
    if (file.type == NONE)
        return 0;
    uint32_t defragmented_files;
    if (file.type == FILE)
        defragmented_files = defragment_file(file);

    if (file.type == DIR || file.type == ROOT_DIR)
        defragmented_files = defragment_dir(file);

    return defragmented_files;
}

uint32_t Partition::defragment_dir(FileInfo& file)
{
    if (file.type != DIR && file.type != ROOT_DIR)
        return 0;
    auto fat_type = m_pbr.get_parameters().fat_type;
    assert((fat_type == PBR::FAT12 || fat_type == PBR::FAT16
        || fat_type == PBR::FAT32) && "Invalid partition type.");

    uint32_t counter = 0;
    Bytes buff;
    auto cluster_size = m_pbr.get_parameters().cluster_size;
    auto data_offset = m_pbr.get_parameters().data_offset;
    auto root_dir_size = m_pbr.get_parameters().root_dir_size;

    if (file.type == ROOT_DIR && fat_type != PBR::FAT32)
    {
        buff.resize(root_dir_size);
        m_drive.seekg(data_offset, m_drive.beg);
        m_drive.read(buff, root_dir_size);
        counter = defragment_dir_cluster(buff, 
            m_pbr.get_parameters().root_dir_cluster);
    }
    else
    {
        uint8_t shift = 1U;
        uint64_t offset;
        if (fat_type == PBR::FAT16 || fat_type == PBR::FAT12)
        {
            data_offset += m_pbr.get_parameters().root_dir_size;
            shift = 2U;
        }
        buff.resize(cluster_size);
        uint32_t current_cluster;
        uint32_t next_cluster = file.first_cluster;
        do
        {
            current_cluster = next_cluster;
            next_cluster = m_FAT.get_value<uint32_t>
                (next_cluster * 2, Bytes::WORD);

            offset = data_offset + (current_cluster - shift) * cluster_size;
            m_drive.seekg(offset, m_drive.beg);
            m_drive.read(buff, cluster_size);
            counter += defragment_dir_cluster(buff, current_cluster);
            
        } while (next_cluster != 0xFFFFU);
    }
    return counter;
}


uint32_t Partition::defragment_dir_cluster(Bytes& dir_cluster, uint32_t cluster_number)
{
    uint32_t counter = 0;
    FileInfo file;
    file.partition_sn = m_pbr.get_parameters().serial_number;
    unsigned char ch;
    for (int32_t i = 0; i < dir_cluster.length(); i += 0x20U)
    {
        ch = dir_cluster.get_value<unsigned char>(i, Bytes::BYTE);
        if (ch == 0)
            break;
        if (ch == 0xE5U || ch == '.')
            continue;
        file = get_file_from_entry(dir_cluster, cluster_number, i);
        if (file.type != NONE)
            counter += defragment_file(file);
    }
    return counter;
}


uint32_t Partition::defragment_file(FileInfo& file)
{
    if (file.partition_sn != m_pbr.get_parameters().serial_number
        || file.type == NONE || file.type == ROOT_DIR)
    {
        //std::cout << "Некорректный файл\n";
        return 0;
    }
    if (!is_file_fragmented(file))
        return 0;
    uint32_t clusters_per_file = count_file_clusters(file); 
    uint32_t first_free_cluster = find_empty_space(clusters_per_file);
    if (first_free_cluster == 0)
    {
        //std::cout << "Недостаточно свободного места для дефрагментации.\n";
        return 0;
    }

    // Копирование кластеров данных файла в новое пространство.
    uint32_t src_cluster = file.first_cluster;
    uint32_t dest_cluster = first_free_cluster;
    for (uint32_t i = 0; i < clusters_per_file; ++i)
    {
        copy_cluster(src_cluster, dest_cluster + i);
        src_cluster = m_FAT.get_value<uint32_t>
            (src_cluster * 2, Bytes::WORD);

        ++dest_cluster;
        if ((i + 1U) == clusters_per_file)
        {
            m_FAT.insert<uint32_t>(0xFFFFU, (dest_cluster - 1U) * 2, Bytes::WORD);
            break;
        }
        m_FAT.insert<uint32_t>(dest_cluster, (dest_cluster - 1U) * 2, Bytes::WORD);
    }
    
    // Стирание старых блоков файла в таблице FAT.
    uint32_t current_src_cluster = file.first_cluster;
    uint32_t previous_src_cluster;
    do {
        previous_src_cluster = current_src_cluster;
        current_src_cluster = m_FAT.get_value<uint32_t>
            (current_src_cluster * 2, Bytes::WORD);
        m_FAT.insert<uint32_t>(0U, previous_src_cluster * 2, Bytes::WORD);
    } while (current_src_cluster != 0xFFFF);

    // Запись таблиц FAT из буфера на накопитель.
    uint64_t fat_offset = m_pbr.get_parameters().fat_offset;
    uint64_t fat_size = m_pbr.get_parameters().fat_size;
    uint64_t cur_fat_offset;
    for (uint8_t i = 0; i < m_pbr.get_parameters().fat_number; ++i)
    {
        cur_fat_offset = fat_offset + fat_size * i;
        m_drive.seekp(cur_fat_offset, m_drive.beg);
        m_drive.write(m_FAT, fat_size);
    }
    
    // Запись номера нового первого кластера файла в запись файла.
    Bytes buff(2);
    buff.insert<uint32_t>(first_free_cluster, 0, Bytes::WORD);
    m_drive.seekp(file.entry_offset + 0x1AU, m_drive.beg);
    m_drive.write(buff, 2);

    return 1;
}

void Partition::copy_cluster(uint32_t source, uint32_t destination)
{
    PBR::FAT_Type fat_type = m_pbr.get_parameters().fat_type;
    if (fat_type == PBR::NONE)
    {
        return;
    }
    uint8_t shift;
    uint64_t src_offset;
    uint64_t dest_offset;

    uint32_t cluster_size = m_pbr.get_parameters().cluster_size;
    uint64_t data_offset = m_pbr.get_parameters().data_offset;

    Bytes buff;

    if (fat_type == PBR::FAT12 || fat_type == PBR::FAT16)
    {
        shift = 2U;
        src_offset = data_offset + m_pbr.get_parameters().root_dir_size 
            + cluster_size * (source - shift);
        dest_offset = data_offset + m_pbr.get_parameters().root_dir_size 
            + cluster_size * (destination - shift);
        buff.resize(cluster_size);
        m_drive.seekg(src_offset, m_drive.beg);
        m_drive.read(buff, cluster_size);
        m_drive.seekp(dest_offset, m_drive.beg);
        m_drive.write(buff, cluster_size);
    }
    if (fat_type == PBR::FAT32)
    {
        shift = 1U;
        src_offset = data_offset
            + cluster_size * (source - shift);
        dest_offset = data_offset
            + cluster_size * (destination - shift);
        
        buff.resize(cluster_size);
        m_drive.seekg(src_offset, m_drive.beg);
        m_drive.read(buff, cluster_size);
        m_drive.seekp(dest_offset, m_drive.beg);
        m_drive.write(buff, cluster_size);
    }
}

// Принимает на вход количество кластеров, необходимых файлу,
// и возвращает номер первого кластера, в который можно производить запись.
uint32_t Partition::find_empty_space(uint32_t clusters_number)
{
    uint32_t fat_block;
    uint32_t counter = 0;
    uint32_t first_cluster = 0;
    uint32_t block_size = 2U;
    uint32_t last_data_cluster = 0xFFEFU;
    // Первые два блока (0, 1) зарезервированы. Третий (2) не используется.
    for (uint32_t i = 3; i < m_FAT.length(); ++i)
    {
        if (counter == clusters_number) break;

        if ((i / block_size) > last_data_cluster)
        {
            first_cluster = 0;
            break;
        } 
        fat_block = m_FAT.get_value<uint32_t>(i * block_size, Bytes::WORD);
        if (fat_block == 0)
        {
            if (counter == 0) 
                first_cluster = i;    
            ++counter;
            continue;
        }
        if (counter > 0)
        {
            counter = 0;
            first_cluster = 0;
        }
    }
    return first_cluster;
}

uint32_t Partition::count_file_clusters(const FileInfo& file)
{   
    if (file.first_cluster == 0)
        return 0U;
    uint32_t counter = 0;
    uint32_t current_cluster = file.first_cluster;
    do
    {
        ++counter;
        current_cluster = m_FAT.get_value<uint32_t>
            (current_cluster * 2, Bytes::WORD);
    } while (current_cluster != 0xFFFF);
    return counter;
}
