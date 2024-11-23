
#include <iostream>
#include <fstream>
#include <cstdint>
#include <iomanip>

#include "PBR.h"

namespace Constants
{
    namespace Offsets
    {
        extern const size_t bytes_per_sector = 0x0B;
        extern const size_t sectors_per_cluster = 0x0D;
        extern const size_t reserved_sectors = 0x0E;
        extern const size_t fat_number = 0x10;
        extern const size_t root_entries = 0x11;
        extern const size_t small_sector_count = 0x13;
        extern const size_t sectors_per_fat_12_16 = 0x16;
        extern const size_t hidden_sectors = 0x1C;
        extern const size_t large_sector_count = 0x20;
        extern const size_t sectors_per_fat_32 = 0x24;
        extern const size_t root_dir_cluster = 0x2C;
        extern const size_t serial_number_12_16 = 0x27;
        extern const size_t serial_number_32 = 0x43;
        extern const size_t volume_label_12_16 = 0x2B;
        extern const size_t volume_label_32 = 0x47;
        extern const size_t fat_signature_12_16 = 0x26;
        extern const size_t fat_signature_32 = 0x42;
        extern const size_t signature = 0x1FE;
    }
    namespace Sizes
    {
        extern const Bytes::TypeSize bytes_per_sector = Bytes::WORD;
        extern const Bytes::TypeSize sectors_per_cluster = Bytes::BYTE;
        extern const Bytes::TypeSize reserved_sectors = Bytes::WORD;
        extern const Bytes::TypeSize fat_number = Bytes::BYTE;
        extern const Bytes::TypeSize root_entries = Bytes::WORD;
        extern const Bytes::TypeSize small_sector_count = Bytes::WORD;
        extern const Bytes::TypeSize sectors_per_fat_12_16 = Bytes::WORD;
        extern const Bytes::TypeSize hidden_sectors = Bytes::DOUBLE_WORD;
        extern const Bytes::TypeSize large_sector_count = Bytes::DOUBLE_WORD;
        extern const Bytes::TypeSize sectors_per_fat_32 = Bytes::DOUBLE_WORD;
        extern const Bytes::TypeSize root_dir_cluster = Bytes::DOUBLE_WORD;
        extern const Bytes::TypeSize serial_number_12_16 = Bytes::DOUBLE_WORD;
        extern const Bytes::TypeSize serial_number_32 = Bytes::DOUBLE_WORD;
        extern const uint8_t volume_label_12_16 = 11;
        extern const uint8_t volume_label_32 = 11;
        extern const Bytes::TypeSize fat_signature_12_16 = Bytes::BYTE;
        extern const Bytes::TypeSize fat_signature_32 = Bytes::BYTE;
        extern const uint16_t partition_boot_record = 512;
        extern const Bytes::TypeSize signature = Bytes::WORD;
    }
    namespace Values
    {
        extern const uint16_t signature = 0xAA55;
        extern const uint8_t fat_signature_0x28 = 0x28;
        extern const uint8_t fat_signature_0x29 = 0x29;
        extern const uint8_t root_entry = 32;
        extern const uint32_t root_dir_cluster_12_16 = 1;
        extern const uint64_t Gb = 1'073'741'824;
        extern const uint32_t Mb = 1'048'576;
    }
}

PBR::PBR() : m_parameters({})
{
}

PBR::PBR(const std::string path, uint64_t offset) 
{
    init(path, offset);
}

void PBR::init(const std::string& path, uint64_t offset)
{
    m_parameters = {};
    std::fstream file;
    file.open(path, std::ios::binary | std::ios::in | std::ios::out);
    if (file.is_open())
    {
        namespace CS = Constants::Sizes;
        m_buff.resize(CS::partition_boot_record);
        file.seekg(offset, file.beg);
        file.read(m_buff, CS::partition_boot_record);
        if (is_pbr())
        {
            set_pbr(offset);
        }

        file.close();
    }
}

void PBR::init(std::fstream& drive)
{
    namespace CS = Constants::Sizes;
    m_parameters = {};
    m_buff.resize(CS::partition_boot_record);
    drive.seekg(0, drive.beg);
    drive.read(m_buff, CS::partition_boot_record);
    if (is_pbr())
    {
        set_pbr();
    }
}

PBR& PBR::clear()
{
    m_buff.clear();
    m_parameters = {};
    return *this;
}

void PBR::set(const std::string& path, uint64_t offset)
{
    clear();
    init(path, offset);
}

void PBR::set(std::fstream& drive)
{
    clear();
    init(drive);
}

bool PBR::is_pbr() const
{
    namespace CO = Constants::Offsets;
    namespace CS = Constants::Sizes;
    namespace CV = Constants::Values;

    uint16_t sign = 0;
    uint8_t fat_sign_12_16 = 0;
    uint8_t fat_sign_32 = 0;

    if (m_buff.length() == CS::partition_boot_record)
    {
        sign = m_buff.get_value<uint16_t>
            (CO::signature, CS::signature);
        if (sign != CV::signature)
            return false;

        fat_sign_12_16 = m_buff.get_value<uint8_t>
            (CO::fat_signature_12_16, CS::fat_signature_12_16);
        fat_sign_32 = m_buff.get_value<uint8_t>
            (CO::fat_signature_32, CS::fat_signature_32);
    
        if (   fat_sign_12_16 == CV::fat_signature_0x28 
            || fat_sign_12_16 == CV::fat_signature_0x29 
            || fat_sign_32 == CV::fat_signature_0x28
            || fat_sign_32 == CV::fat_signature_0x29 )
            return true;
    }
    return false;
}

void PBR::set_pbr(uint64_t offset)
{
    namespace CO = Constants::Offsets;
    namespace CS = Constants::Sizes;
    namespace CV = Constants::Values;

    uint16_t b_p_s = m_buff.get_value<uint16_t>
        ( CO::bytes_per_sector, CS::bytes_per_sector );


    uint8_t  s_p_c = m_buff.get_value<uint8_t>
        ( CO::sectors_per_cluster, CS::sectors_per_cluster );

    uint16_t fs_12_16 = m_buff.get_value<uint16_t>
        ( CO::sectors_per_fat_12_16, CS::sectors_per_fat_12_16);

    uint32_t fs_32 = m_buff.get_value<uint32_t>
            ( CO::sectors_per_fat_32, CS::sectors_per_fat_32 );

    uint16_t s_s_c = m_buff.get_value<uint16_t>
        ( CO::small_sector_count, CS::small_sector_count );

    uint32_t l_s_c = m_buff.get_value<uint32_t>
        ( CO::large_sector_count, CS::large_sector_count ); 

    uint64_t hidden_sectors = 0;
    if (offset != 0)
        hidden_sectors = m_buff.get_value<uint64_t>
            ( CO::hidden_sectors, CS::hidden_sectors );

    uint16_t root_entries = m_buff.get_value<uint16_t>
        ( CO::root_entries, CS::root_entries );
    
    /*
    std::cout   << std::hex     << std::uppercase 
                << "bites per sector: "         << b_p_s << '\n'
                << "sectors_per_cluster: "      << static_cast<uint16_t>(s_p_c) << '\n'
                << "sectors_per_fat_12_16: "    << fs_12_16 << '\n'
                << "sectors_per_fat_32: "       << fs_32 << '\n'
                << "small_sector_count: "       << s_s_c << '\n'
                << "large_sector_count: "       << l_s_c << '\n'
                << "hidden_sectors: "           << hidden_sectors << '\n'
                << "root_entries: "             << root_entries << '\n';
    std::cout   << std::dec << '\n';
    */

    m_parameters.cluster_size = static_cast<uint32_t>(b_p_s) * s_p_c;

    m_parameters.fat_offset = b_p_s * ( hidden_sectors
        + m_buff.get_value<uint64_t>
        ( CO::reserved_sectors, CS::reserved_sectors ) );

    m_parameters.fat_number = m_buff.get_value<uint8_t>
        ( CO::fat_number, CS::fat_number );

    if (fs_12_16 == 0) 
        m_parameters.fat_size = static_cast<uint64_t>(fs_32) * b_p_s;
    else
        m_parameters.fat_size = static_cast<uint64_t>(fs_12_16) * b_p_s;
    
    m_parameters.data_offset = m_parameters.fat_offset  
        + m_parameters.fat_size * m_parameters.fat_number;

    if (s_s_c == 0)
        m_parameters.partition_size = static_cast<uint64_t>(l_s_c) * b_p_s;
    else
        m_parameters.partition_size = static_cast<uint64_t>(s_s_c) * b_p_s;

    m_parameters.root_dir_size = root_entries * CV::root_entry;

    m_parameters.clusters_number = (m_parameters.partition_size 
            - m_parameters.data_offset + hidden_sectors 
            - m_parameters.root_dir_size) / m_parameters.cluster_size;

    if (fs_12_16 != 0)
    {
        if (m_parameters.clusters_number < 4085)
            m_parameters.fat_type = FAT12;
        else if (   m_parameters.clusters_number >= 4085 
                &&  m_parameters.clusters_number <= 65524 )
            m_parameters.fat_type = FAT16;
    }
    if (m_parameters.clusters_number > 65524 && fs_12_16 == 0) 
        m_parameters.fat_type = FAT32;

    if (m_parameters.fat_type == FAT32)
        m_parameters.root_dir_cluster = m_buff.get_value<uint32_t>
            ( CO::root_dir_cluster, CS::root_dir_cluster);
    else if (m_parameters.fat_type == FAT12 || m_parameters.fat_type == FAT16)
        m_parameters.root_dir_cluster = 1;

    m_parameters.last_cluster = m_parameters.clusters_number + 1; // root_dir

    if (m_parameters.fat_type == FAT12 || m_parameters.fat_type == FAT16)
        m_parameters.serial_number = m_buff.get_value<uint32_t>
            (CO::serial_number_12_16, CS::serial_number_12_16);
    else
        m_parameters.serial_number = m_buff.get_value<uint32_t>
            (CO::serial_number_32, CS::serial_number_32);

    if (m_parameters.fat_type == FAT12 || m_parameters.fat_type == FAT16)
        m_parameters.label = m_buff.get_string
            (CO::volume_label_12_16, CS::volume_label_12_16);
    if (m_parameters.fat_type == FAT32)
        m_parameters.label = m_buff.get_string
            (CO::volume_label_32, CS::volume_label_32);    
}

bool PBR::is_fat() const
{
    if (m_parameters.fat_type != NONE)
        return true;
    else
        return false;
}

void PBR::print_parameters() const
{
    namespace CV = Constants::Values;
    switch(m_parameters.fat_type)
    {
        case FAT12:
            std::cout << "type: FAT12\n";
            break;
        case FAT16:
            std::cout << "type: FAT16\n";
            break;
        case FAT32:
            std::cout << "type: FAT32\n";
            break;
        default:
            std::cout << "It's not fat.\n";
            break;
    }
    std::cout << "label: " << m_parameters.label << '\n';
    std::cout << "partition size: " << m_parameters.partition_size << " Bytes\n";
    std::cout << "cluster size: " << m_parameters.cluster_size << " Bytes\n";
    std::cout << "fat number: " << static_cast<uint16_t>(m_parameters.fat_number) << '\n';
    std::cout << "fat size: " << m_parameters.fat_size << " Bytes\n";
    std::cout << "fat offset: " << m_parameters.fat_offset << '\n';
    std::cout << "data offset: " << m_parameters.data_offset << '\n';
    std::cout << "root_dir size: " << m_parameters.root_dir_size << " Bytes\n";
    std::cout << "root_dir_cluster: " << m_parameters.root_dir_cluster << "\n";
    std::cout << "clusters_number: " << m_parameters.clusters_number << "\n";
    std::cout << "last_cluster: " << m_parameters.last_cluster << "\n";
}

void PBR::print() const
{
    namespace CV = Constants::Values;
    std::cout << "________________________________________\n"; // 40
    std::cout << "Имя: " << m_parameters.label << '\n';

    std::cout << std::setprecision(2) << std::fixed;
    if (m_parameters.partition_size > CV::Gb)
        std::cout << "Размер: " << m_parameters.partition_size 
                                / static_cast<double>(CV::Gb) << "Gb\n";
    else
        std::cout << "Размер: " << m_parameters.partition_size 
                                / static_cast<double>(CV::Mb) << "Mb\n";

    std::cout << "Доступная память: " 
        << m_parameters.clusters_number * m_parameters.cluster_size 
            / static_cast<double>(CV::Mb) << " Mb\n";
    std::cout << std::resetiosflags(std::ios::fixed);

    if (m_parameters.fat_type == FAT12)
        std::cout << "Тип файловой системы: FAT12\n";
    if (m_parameters.fat_type == FAT16)
        std::cout << "Тип файловой системы: FAT16\n";
    if (m_parameters.fat_type == FAT32)
        std::cout << "Тип файловой системы: FAT32\n";
    std::cout <<   "========================================\n\n"; // 40
}
