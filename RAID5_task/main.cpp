#ifndef __PROGTEST__

#include <cstdio>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <cassert>
using namespace std;

const int SECTOR_SIZE = 512;
const int MAX_RAID_DEVICES = 16;
const int MAX_DEVICE_SECTORS = 1024 * 1024 * 2;
const int MIN_DEVICE_SECTORS = 1 * 1024 * 2;

const int RAID_STOPPED = 0;
const int RAID_OK = 1;
const int RAID_DEGRADED = 2;
const int RAID_FAILED = 3;


struct TBlkDev {
    int m_Devices;
    int m_Sectors;

    int (* m_Read)(int, int, void *, int);
    int (* m_Write)(int, int, const void *, int);
};

#endif /* __PROGTEST__ */

#define BYTE unsigned char
const int DEFAULT_BAD_DISC = -1;
const int DEFAULT_TIMESTAMP = 1;

struct ServiceData{
    int m_Status;
    int m_BadDisc;
    int m_TimeStamp;
};

class CRaidVolume {
public:
    CRaidVolume()
        : m_Dev(), m_Data(){
        m_Data.m_Status = RAID_STOPPED;
    }

    static bool Create(const TBlkDev &dev){
        ServiceData data = {RAID_OK, DEFAULT_BAD_DISC, DEFAULT_TIMESTAMP};
        for (int i = 0; i < dev.m_Devices; i++)
            if (dev.m_Write(i, dev.m_Sectors - 1, &data, 1) != 1)
                return false;
        return true;
    }

    int Start(const TBlkDev &dev){
        if (m_Data.m_Status != RAID_STOPPED) return m_Data.m_Status;
        m_Dev = dev;

        BYTE buffer [SECTOR_SIZE];
        int service [3];
        ServiceData start_data [3];
        int fail_counter = 0, bad_disc = DEFAULT_BAD_DISC;

        for (int i = 0; i < 3; i++){
            if (m_Dev.m_Read(i, m_Dev.m_Sectors - 1, buffer, 1) != 1){
                fail_counter++; bad_disc = i;
                continue;
            }
            memcpy(service, buffer, 3 * sizeof(int));
            start_data[i].m_Status = service[0];
            start_data[i].m_BadDisc = service[1];
            start_data[i].m_TimeStamp = service[2];
        }

        if (start_data[0].m_TimeStamp == start_data[1].m_TimeStamp
         && start_data[1].m_TimeStamp == start_data[2].m_TimeStamp)
            m_Data = start_data[0];
        else if (start_data[0].m_TimeStamp != start_data[1].m_TimeStamp
              && start_data[1].m_TimeStamp != start_data[2].m_TimeStamp
              && start_data[0].m_TimeStamp != start_data[2].m_TimeStamp){
            m_Data.m_BadDisc = DEFAULT_BAD_DISC;
            m_Data.m_TimeStamp = DEFAULT_TIMESTAMP;
            return m_Data.m_Status = RAID_FAILED;
        } else
            m_Data = (start_data[0].m_TimeStamp != start_data[1].m_TimeStamp
                   && start_data[1].m_TimeStamp != start_data[2].m_TimeStamp) ? start_data[0] : start_data[1];

        for (int i = 3; i < m_Dev.m_Devices; i++){
            if (m_Dev.m_Read(i, m_Dev.m_Sectors - 1, buffer, 1) != 1){
                if (m_Data.m_Status == RAID_OK){
                    m_Data.m_Status = RAID_DEGRADED; m_Data.m_BadDisc = i;
                    continue;
                } else if (m_Data.m_Status == RAID_DEGRADED && m_Data.m_BadDisc != i){
                    m_Data.m_BadDisc = DEFAULT_BAD_DISC;
                    return m_Data.m_Status = RAID_FAILED;
                }
            }
            memcpy(service, buffer, 3 * sizeof(int));
            if (m_Data.m_Status != service[0] || m_Data.m_BadDisc != service[1] || m_Data.m_TimeStamp != service[2]){
                if (m_Data.m_Status == RAID_OK){
                    m_Data.m_Status = RAID_DEGRADED; m_Data.m_BadDisc = i;
                } else if (m_Data.m_Status == RAID_DEGRADED && m_Data.m_BadDisc != i){
                    m_Data.m_BadDisc = DEFAULT_BAD_DISC;
                    return m_Data.m_Status = RAID_FAILED;
                }
            }
        }

        if (fail_counter > 0){
            m_Data.m_Status += fail_counter;
            if (m_Data.m_Status == RAID_FAILED){
                m_Data.m_BadDisc = DEFAULT_BAD_DISC;
                return m_Data.m_Status;
            }
            m_Data.m_BadDisc = bad_disc;
        }

        return m_Data.m_Status;
    }

    int Stop(){
        m_Data.m_TimeStamp++;
        for (int i = 0; i < m_Dev.m_Devices; i++)
            m_Dev.m_Write(i, m_Dev.m_Sectors - 1, &m_Data, 1);

        m_Data.m_BadDisc = DEFAULT_BAD_DISC;
        return m_Data.m_Status = RAID_STOPPED;
    }

    int Resync(){
        if (m_Data.m_Status != RAID_DEGRADED)
            return m_Data.m_Status;

        BYTE sector [SECTOR_SIZE];
        for (int i = 0; i < m_Dev.m_Sectors; i++){
            if (!restore_sector(sector, i)) {
                m_Data.m_BadDisc = DEFAULT_BAD_DISC;
                return m_Data.m_Status = RAID_FAILED;
            }
            if (m_Dev.m_Write(m_Data.m_BadDisc, i, (void *)sector, 1) != 1)
                return m_Data.m_Status;
        }

        m_Data.m_BadDisc = DEFAULT_BAD_DISC;
        return m_Data.m_Status = RAID_OK;
    }

    int Status() const{ return m_Data.m_Status; }

    int Size() const{ return (m_Dev.m_Devices - 1) * (m_Dev.m_Sectors - 1); }

    bool Read(int secNr, void *data, int secCnt){
        if (m_Data.m_Status == RAID_STOPPED || m_Data.m_Status == RAID_FAILED) return false;

        BYTE read_sector [SECTOR_SIZE];

        for (int i = 0; i < secCnt; i++){
            int raid_device_num, raid_sector_num;
            convert_index(secNr + i, raid_device_num, raid_sector_num);

            if (m_Data.m_Status == RAID_OK)
                if (m_Dev.m_Read(raid_device_num, raid_sector_num, (void *)read_sector, 1) != 1){
                    m_Data.m_Status = RAID_DEGRADED; m_Data.m_BadDisc = raid_device_num;
                }

            if (m_Data.m_Status == RAID_DEGRADED){
                if (m_Data.m_BadDisc == raid_device_num) {
                    if (!restore_sector(read_sector, raid_sector_num)) {
                        m_Data.m_Status = RAID_FAILED; m_Data.m_BadDisc = DEFAULT_BAD_DISC;
                        return false;
                    }
                } else {
                    if (m_Dev.m_Read(raid_device_num, raid_sector_num, (void *) read_sector, 1) != 1){
                        m_Data.m_Status = RAID_FAILED; m_Data.m_BadDisc = DEFAULT_BAD_DISC;
                        return false;
                    }
                }
            }

            // appending the read or computed data to the output parameter
            memcpy((void *)((BYTE *)data + i * SECTOR_SIZE), read_sector, SECTOR_SIZE * sizeof(BYTE));
        }
        return true;
    }

    bool Write(int secNr, const void *data, int secCnt){
        if (m_Data.m_Status == RAID_STOPPED || m_Data.m_Status == RAID_FAILED) return false;

        for (int i = 0; i < secCnt; i++) {
            // read the old sector
            BYTE old_sector [SECTOR_SIZE], parity [SECTOR_SIZE], new_sector [SECTOR_SIZE];
            if (!Read(secNr + i, old_sector, 1))
                return false;

            // read the old parity
            int raid_device_num, raid_sector_num;
            convert_index(secNr + i, raid_device_num, raid_sector_num);
            int parity_device_num = raid_sector_num % m_Dev.m_Devices;

            if (m_Data.m_Status == RAID_OK)
                if (m_Dev.m_Read(parity_device_num, raid_sector_num, (void *)parity, 1) != 1){
                    m_Data.m_Status = RAID_DEGRADED; m_Data.m_BadDisc = parity_device_num;
                }

            if (m_Data.m_Status == RAID_DEGRADED){
                if (m_Data.m_BadDisc == parity_device_num) {
                    if (!restore_sector(parity, raid_sector_num)) {
                        m_Data.m_Status = RAID_FAILED; m_Data.m_BadDisc = DEFAULT_BAD_DISC;
                        return false;
                    }
                } else {
                    if (m_Dev.m_Read(parity_device_num, raid_sector_num, (void *)parity, 1) != 1) {
                        m_Data.m_Status = RAID_FAILED; m_Data.m_BadDisc = DEFAULT_BAD_DISC;
                        return false;
                    }
                }
            }

            // get the new sector
            memcpy(new_sector, (void *)((BYTE*)data + i * SECTOR_SIZE), SECTOR_SIZE);

            // compute the new parity
            for (int j = 0; j < SECTOR_SIZE; j++)
                (parity[j] ^= old_sector[j]) ^= new_sector[j];

            //write the new sector
            if (m_Data.m_Status == RAID_OK
            || (m_Data.m_Status == RAID_DEGRADED && m_Data.m_BadDisc != raid_device_num))
                m_Dev.m_Write(raid_device_num, raid_sector_num, (void *)new_sector, 1);

            //write the new parity
            if (m_Data.m_Status == RAID_OK
            || (m_Data.m_Status == RAID_DEGRADED && m_Data.m_BadDisc != parity_device_num))
                m_Dev.m_Write(parity_device_num, raid_sector_num, (void *)parity, 1);
        }
        return true;
    }

protected:
    TBlkDev m_Dev;
    ServiceData m_Data;

    void convert_index(int idx, int & device, int & sector) const{
        int shift = m_Dev.m_Devices * (m_Dev.m_Devices - 1);
        idx += (int) (idx / m_Dev.m_Devices) + 1 + (int)(idx / shift);
        device = idx % m_Dev.m_Devices;
        sector = (int) (idx / m_Dev.m_Devices);
    }

    bool restore_sector(BYTE * damaged_sector, int raid_sector_num) const {
        assert(m_Data.m_Status == RAID_DEGRADED && m_Data.m_BadDisc != DEFAULT_BAD_DISC);

        // read the row of sectors to restore the damaged one
        auto * row_of_sectors = (BYTE *)malloc(SECTOR_SIZE * m_Dev.m_Devices * sizeof(BYTE));
        //BYTE row_of_sectors [SECTOR_SIZE * MAX_RAID_DEVICES];
        for (int j = 0; j < m_Dev.m_Devices; j++){
            if (j == m_Data.m_BadDisc) continue;
            if (m_Dev.m_Read(j, raid_sector_num, (void *)(row_of_sectors + SECTOR_SIZE * j), 1) != 1)
                return false;
        }

        // restore it
        for (int k = 0; k < SECTOR_SIZE; k++){
            int first_idx = 0, second_idx = 1;
            if (m_Data.m_BadDisc == first_idx){
                first_idx = 1; second_idx = 2;
            } else if (m_Data.m_BadDisc == second_idx)
                second_idx = 2;

            row_of_sectors[SECTOR_SIZE * m_Data.m_BadDisc + k] =
                    row_of_sectors[SECTOR_SIZE * first_idx + k] ^ row_of_sectors[SECTOR_SIZE * second_idx + k];

            for (int l = second_idx + 1; l < m_Dev.m_Devices; l++){
                if (l == m_Data.m_BadDisc) continue;
                row_of_sectors[SECTOR_SIZE * m_Data.m_BadDisc + k] ^= row_of_sectors[SECTOR_SIZE * l + k];
            }
        }

        memcpy(damaged_sector, row_of_sectors + SECTOR_SIZE * m_Data.m_BadDisc, SECTOR_SIZE);
        free(row_of_sectors);
        return true;
    }
};


#ifndef __PROGTEST__

#include "tests.inc"

#endif /* __PROGTEST__ */
