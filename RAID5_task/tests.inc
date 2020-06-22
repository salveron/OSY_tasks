/* SW RAID5 - basic test
 *
 * The testing of the RAID driver requires a backend (simulating the underlying disks).
 * Next, the tests of your RAID implemetnation are needed. To help you with the implementation, 
 * a sample backend is implemented in this file. It provides a quick-and-dirty
 * implementation of the underlying disks (simulated in files) and a few Raid... function calls.
 *
 * The implementation in the real testing environment is different. The sample below is a 
 * minimalistic disk backend which matches the required interface. The backend, for instance, 
 * cannot simulate a crashed disk. To test your Raid implementation, you will have to modify 
 * or extend the backend.
 *
 * Next, you will have to add some raid testing. There is a few Raid... functions called from within 
 * main(), however, the tests are incomplete. For instance, RaidResync () is not tested here. Once 
 * again, this is only a starting point.
 */
#include <iostream>
using namespace std;

const int RAID_DEVICES = 4;
const int DISK_SECTORS = 8192;
static FILE *g_Fp[RAID_DEVICES];

int bad_disc = -1;

//-------------------------------------------------------------------------------------------------
/** Sample sector reading function. The function will be called by your Raid driver implementation.
 * Notice, the function is not called directly. Instead, the function will be invoked indirectly 
 * through function pointer in the TBlkDev structure.
 */
int diskRead(int device, int sectorNr, void *data, int sectorCnt) {
    if (device < 0 || device >= RAID_DEVICES)
        return 0;
    if (g_Fp[device] == NULL)
        return 0;
    if (sectorCnt <= 0 || sectorNr + sectorCnt > DISK_SECTORS)
        return 0;
    if (device == bad_disc)  // simulating the disc failure
        return 0;
    fseek(g_Fp[device], sectorNr * SECTOR_SIZE, SEEK_SET);
    return fread(data, SECTOR_SIZE, sectorCnt, g_Fp[device]);
}
//-------------------------------------------------------------------------------------------------
/** Sample sector writing function. Similar to diskRead
 */
int diskWrite(int device, int sectorNr, const void *data, int sectorCnt) {
    if (device < 0 || device >= RAID_DEVICES)
        return 0;
    if (g_Fp[device] == NULL)
        return 0;
    if (sectorCnt <= 0 || sectorNr + sectorCnt > DISK_SECTORS)
        return 0;
    if (device == bad_disc)  // simulating the disc failure
        return 0;
    fseek(g_Fp[device], sectorNr * SECTOR_SIZE, SEEK_SET);
    return fwrite(data, SECTOR_SIZE, sectorCnt, g_Fp[device]);
}
//-------------------------------------------------------------------------------------------------
/** A function which releases resources allocated by openDisks/createDisks
 */
void doneDisks(void) {
    for (int i = 0; i < RAID_DEVICES; i++)
        if (g_Fp[i]) {
            fclose(g_Fp[i]);
            g_Fp[i] = NULL;
        }
}
//-------------------------------------------------------------------------------------------------
/** A function which creates the files needed for the sector reading/writing functions above.
 * This function is only needed for the particular implementation above. 
 */
TBlkDev createDisks(void) {
    char buffer[SECTOR_SIZE];
    TBlkDev res;
    char fn[100];

    memset(buffer, 0, sizeof(buffer));

    for (int i = 0; i < RAID_DEVICES; i++) {
        snprintf(fn, sizeof(fn), "/tmp/%04d", i);
        g_Fp[i] = fopen(fn, "w+b");
        if (!g_Fp[i]) {
            doneDisks();
            throw "Raw storage create error";
        }

        for (int j = 0; j < DISK_SECTORS; j++)
            if (fwrite(buffer, sizeof(buffer), 1, g_Fp[i]) != 1) {
                doneDisks();
                throw "Raw storage create error";
            }
    }

    res.m_Devices = RAID_DEVICES;
    res.m_Sectors = DISK_SECTORS;
    res.m_Read = diskRead;
    res.m_Write = diskWrite;
    return res;
}
//------------------------------------------------------------------------------------------------- 
/** A function which opens the files needed for the sector reading/writing functions above.
 * This function is only needed for the particular implementation above. 
 */
TBlkDev openDisks(void) {
    TBlkDev res;
    char fn[100];

    for (int i = 0; i < RAID_DEVICES; i++) {
        snprintf(fn, sizeof(fn), "/tmp/%04d", i);
        g_Fp[i] = fopen(fn, "r+b");
        if (!g_Fp[i]) {
            doneDisks();
            throw "Raw storage access error";
        }
        fseek(g_Fp[i], 0, SEEK_END);
        if (ftell(g_Fp[i]) != DISK_SECTORS * SECTOR_SIZE) {
            doneDisks();
            throw "Raw storage read error";
        }
    }
    res.m_Devices = RAID_DEVICES;
    res.m_Sectors = DISK_SECTORS;
    res.m_Read = diskRead;
    res.m_Write = diskWrite;
    return res;
}

//-------------------------------------------------------------------------------------------------
void test1(){
    /* create the disks before we use them
     */
    TBlkDev dev = createDisks();
    /* The disks are ready at this moment. Your RAID-related functions may be executed,
     * the disk backend is ready.
     *
     * First, try to create the RAID:
     */
    assert(CRaidVolume::Create(dev));


    /* start RAID volume */
    CRaidVolume vol;

    assert(vol.Start(dev) == RAID_OK);
    assert(vol.Status() == RAID_OK);

    /* your raid device shall be up.
     * try to read and write all RAID sectors:
     */

    for (int i = 0; i < vol.Size(); i++) {
        char buffer1[SECTOR_SIZE];

        assert(vol.Read(i, buffer1, 1));
        assert(vol.Write(i, buffer1, 1));
    }

    for (int i = 0; i < vol.Size() - 1; i += 2) {
        char buffer2[SECTOR_SIZE * 2];

        assert(vol.Read(i, buffer2, 2));
        assert(vol.Write(i, buffer2, 2));
    }

    char buffer3[SECTOR_SIZE] = "Hello, World!";
    assert(vol.Write(1000, buffer3, 1));
    assert(vol.Read(1000, buffer3, 1));
    assert(!strcmp(buffer3, "Hello, World!"));


    assert(vol.Stop() == RAID_STOPPED);
    assert(vol.Status() == RAID_STOPPED);
    doneDisks();

    cout << "Test 1: OK" << endl;
}
//-------------------------------------------------------------------------------------------------
void test2(){
    /* The RAID as well as disks are stopped. It corresponds i.e. to the
     * restart of a real computer.
     *
     * after the restart, we will not create the disks, nor create RAID (we do not
     * want to destroy the content). Instead, we will only open/start the devices:
     */

    TBlkDev dev = openDisks();
    CRaidVolume vol;
    assert(vol.Start(dev) == RAID_OK);

    for (int i = 0; i < vol.Size() - 1; i += 2) {
        char buffer2[SECTOR_SIZE * 2];

        assert(vol.Read(i, buffer2, 2));
        assert(vol.Write(i, buffer2, 2));
    }

    char buffer3[SECTOR_SIZE] = "Hello, World!";
    assert(vol.Write(1000, buffer3, 1));
    assert(vol.Read(1000, buffer3, 1));
    assert(!strcmp(buffer3, "Hello, World!"));

    vol.Stop();
    doneDisks();

    cout << "Test 2: OK" << endl;
}
//-------------------------------------------------------------------------------------------------
void test3(){
    TBlkDev dev = openDisks();
    CRaidVolume vol;

    assert(vol.Start(dev) == RAID_OK);

    cout << "Test 3.1: ";
    for (int i = 0; i < vol.Size(); i++) {
        char buffer[SECTOR_SIZE];

        assert(vol.Write(i, buffer, 1));
        assert(vol.Read(i, buffer, 1));
    }

    assert(vol.Status() == RAID_OK);
    cout << "OK" << endl;

    cout << "Test 3.2: ";

    for (int i = 0; i < vol.Size() - (vol.Size() % 10); i += 10) {
        char buffer2[10 * SECTOR_SIZE];
        char tempBuffer[10 * SECTOR_SIZE];
        memset(buffer2, i, 10 * SECTOR_SIZE);
        assert(vol.Write(i, buffer2, 10));
        assert(vol.Status() == RAID_OK);
        assert(vol.Read(i, tempBuffer, 10));
        for (int j = 0; j < 10 * SECTOR_SIZE; j++){
            if (tempBuffer[j] != buffer2[j]){
                cout << "> Temp: '";
                printf("%02x", tempBuffer[j]);
                cout << "', buf: '";
                printf("%02x", buffer2[j]);
                cout << "'; idx: " << j << ", test: " << i << endl;
            }
        }
        assert(memcmp(tempBuffer, buffer2, 10 * SECTOR_SIZE) == 0);
    }

    char tempBuffer[10 * SECTOR_SIZE];
    for (int i = 0; i < vol.Size() - (vol.Size() % 10); i += 10) {
        char buffer3[10 * SECTOR_SIZE];
        memset(tempBuffer, i, 10 * SECTOR_SIZE);
        assert(vol.Read(i, buffer3, 10));
        assert(memcmp(tempBuffer, buffer3, 10 * SECTOR_SIZE) == 0);
    }

    assert(vol.Status() == RAID_OK);
    cout << "OK" << endl;


    cout << "Test 3.3: ";
    char buffer4[5 * SECTOR_SIZE];
    char buffer5[6 * SECTOR_SIZE];

    memset(buffer4, 4, 5 * SECTOR_SIZE);
    memset(buffer5, 5, 6 * SECTOR_SIZE);

    assert(vol.Write(2000, buffer4, 5));
    assert(vol.Write(4000, buffer4, 5));
    assert(vol.Write(3000, buffer4, 5));
    assert(vol.Write(5000, buffer4, 5));
    assert(vol.Write(6000, buffer4, 5));
    assert(vol.Write(7000, buffer4, 5));
    assert(vol.Write(8000, buffer4, 5));
    assert(vol.Write(9000, buffer4, 5));
    assert(vol.Write(10000, buffer4, 5));
    assert(vol.Write(12000, buffer4, 5));

    assert(vol.Write(124, buffer5, 6));
    assert(vol.Write(5443, buffer5, 6));
    assert(vol.Write(1, buffer5, 6));
    assert(vol.Write(1432, buffer5, 6));
    assert(vol.Write(23000, buffer5, 6));
    assert(vol.Write(3333, buffer5, 6));
    assert(vol.Write(18000, buffer5, 6));
    assert(vol.Write(12876, buffer5, 6));
    assert(vol.Write(9787, buffer5, 6));
    assert(vol.Write(24565, buffer5, 6));

    char tempBuffer4[5 * SECTOR_SIZE];
    char tempBuffer5[6 * SECTOR_SIZE];

    assert(vol.Read(2000, tempBuffer4, 5));
    assert(memcmp(buffer4, tempBuffer4, 5 * SECTOR_SIZE) == 0);
    assert(vol.Read(3000, tempBuffer4, 5));
    assert(memcmp(buffer4, tempBuffer4, 5 * SECTOR_SIZE) == 0);
    assert(vol.Read(4000, tempBuffer4, 5));
    assert(memcmp(buffer4, tempBuffer4, 5 * SECTOR_SIZE) == 0);
    assert(vol.Read(5000, tempBuffer4, 5));
    assert(memcmp(buffer4, tempBuffer4, 5 * SECTOR_SIZE) == 0);
    assert(vol.Read(6000, tempBuffer4, 5));
    assert(memcmp(buffer4, tempBuffer4, 5 * SECTOR_SIZE) == 0);
    assert(vol.Read(7000, tempBuffer4, 5));
    assert(memcmp(buffer4, tempBuffer4, 5 * SECTOR_SIZE) == 0);
    assert(vol.Read(8000, tempBuffer4, 5));
    assert(memcmp(buffer4, tempBuffer4, 5 * SECTOR_SIZE) == 0);
    assert(vol.Read(9000, tempBuffer4, 5));
    assert(memcmp(buffer4, tempBuffer4, 5 * SECTOR_SIZE) == 0);
    assert(vol.Read(10000, tempBuffer4, 5));
    assert(memcmp(buffer4, tempBuffer4, 5 * SECTOR_SIZE) == 0);
    assert(vol.Read(12000, tempBuffer4, 5));
    assert(memcmp(buffer4, tempBuffer4, 5 * SECTOR_SIZE) == 0);

    assert(vol.Read(124, tempBuffer5, 6));
    assert(memcmp(buffer5, tempBuffer5, 6 * SECTOR_SIZE) == 0);
    assert(vol.Read(5443, tempBuffer5, 6));
    assert(memcmp(buffer5, tempBuffer5, 6 * SECTOR_SIZE) == 0);
    assert(vol.Read(1, tempBuffer5, 6));
    assert(memcmp(buffer5, tempBuffer5, 6 * SECTOR_SIZE) == 0);
    assert(vol.Read(1432, tempBuffer5, 6));
    assert(memcmp(buffer5, tempBuffer5, 6 * SECTOR_SIZE) == 0);
    assert(vol.Read(23000, tempBuffer5, 6));
    assert(memcmp(buffer5, tempBuffer5, 6 * SECTOR_SIZE) == 0);
    assert(vol.Read(3333, tempBuffer5, 6));
    assert(memcmp(buffer5, tempBuffer5, 6 * SECTOR_SIZE) == 0);
    assert(vol.Read(18000, tempBuffer5, 6));
    assert(memcmp(buffer5, tempBuffer5, 6 * SECTOR_SIZE) == 0);
    assert(vol.Read(12876, tempBuffer5, 6));
    assert(memcmp(buffer5, tempBuffer5, 6 * SECTOR_SIZE) == 0);
    assert(vol.Read(9787, tempBuffer5, 6));
    assert(memcmp(buffer5, tempBuffer5, 6 * SECTOR_SIZE) == 0);
    assert(vol.Read(24565, tempBuffer5, 6));
    assert(memcmp(buffer5, tempBuffer5, 6 * SECTOR_SIZE) == 0);

    assert(vol.Status() == RAID_OK);
    cout << "OK" << endl;

    vol.Stop();
    doneDisks();
}
//-------------------------------------------------------------------------------------------------
void test4(){
    bad_disc = 3;

    TBlkDev dev = openDisks();
    CRaidVolume vol;
    assert(vol.Start(dev) == RAID_DEGRADED);
    assert(vol.Status() == RAID_DEGRADED);

    char buffer [SECTOR_SIZE], tempBuffer [SECTOR_SIZE];
    memset(buffer, 1, SECTOR_SIZE);
    memset(tempBuffer, 0, SECTOR_SIZE);

    for (int i = 0; i < RAID_DEVICES; i++){
        if (i == bad_disc) continue;
        assert(vol.Write(i, buffer, 1));
        assert(vol.Read(i, tempBuffer, 1));
        for (int j = 0; j < SECTOR_SIZE; j++){
            if (tempBuffer[j] != buffer[j]){
                printf("Temp: '%02x', buf: '%02x', idx: %d\n", tempBuffer[j], buffer[j], j);
            }
        }
        assert(memcmp(buffer, tempBuffer, SECTOR_SIZE) == 0);
    }

    assert(vol.Status() == RAID_DEGRADED);

    memset(buffer, 2, SECTOR_SIZE);
    assert(vol.Write(bad_disc, buffer, 1));
    assert(vol.Read(bad_disc, tempBuffer, 1));
    assert(memcmp(buffer, tempBuffer, SECTOR_SIZE) == 0);

    vol.Stop();
    doneDisks();

    cout << "Test 4: OK" << endl;
}
//-------------------------------------------------------------------------------------------------
void test5(){
    TBlkDev dev = openDisks();
    CRaidVolume vol;
    assert(vol.Start(dev) == RAID_DEGRADED);
    assert(vol.Status() == RAID_DEGRADED);

    bad_disc = DEFAULT_BAD_DISC;

    assert(vol.Status() == RAID_DEGRADED);
    assert(vol.Resync() == RAID_OK);
    assert(vol.Status() == RAID_OK);

    vol.Stop();
    doneDisks();

    cout << "Test 5: OK" << endl;
}
//-------------------------------------------------------------------------------------------------
int main() {
    test1();
    test2();
    test3();
    test4();
    test5();
    return 0;
}
