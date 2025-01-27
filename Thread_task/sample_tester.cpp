#include <cstdlib>
#include <cstdio>
#include <chrono>
#include <thread>
#include "sample_tester.h"

using namespace std;

//-------------------------------------------------------------------------------------------------
initializer_list<CSampleData> g_TestSets = {{0,  "538",                                                                      {0x0009000001ea, 0x000b00000807, 0x000300000006, 0x000a000003e7}},
                                            {1,  "3248",                                                                     {0x00290000035b, 0x002300000001, 0x00260000004c, 0x002500000009, 0x002a00000199}},
                                            {2,  "38784",                                                                    {0x0047000000b5, 0x004700000024, 0x004300000006, 0x004700000045, 0x004a000004d5}},
                                            {3,  "511576",                                                                   {0x0067000000bd, 0x006b00000d66, 0x0073000c98b2}},
                                            {4,  "4858450",                                                                  {0x008500000002, 0x0089000002d7, 0x008c0000142d, 0x008b00000ba2}},
                                            {5,  "50002924",                                                                 {0x00b00000a464, 0x00b8007fd861}},
                                            {6,  "465976188",                                                                {0x00c700000037, 0x00c8000000cf, 0x00c9000001f7, 0x00c50000000b, 0x00c900000328}},
                                            {7,  "4508409600",                                                               {0x00f5001ae824, 0x00f5000a29b2}},
                                            {8,  "56424486468",                                                              {0x010b00000f52, 0x010a0000034d, 0x010e00004c56, 0x010600000002}},
                                            {9,  "733953135984",                                                             {0x012f0000d3c5, 0x012500000001, 0x012300000007, 0x01250000003a, 0x012d00002b8c}},
                                            {10, "4596194214608",                                                            {0x014b00000bef, 0x014c00000490, 0x014b00000456, 0x0149000001da}},
                                            {11, "86737429983904",                                                           {0x017f4cb42a68, 0x016f0000edfb}},
                                            {12, "585992130990112",                                                          {0x0193000f19c5, 0x019c05a5ae58}},
                                            {13, "9096640217670876",                                                         {0x01a90000034c, 0x01aa000007a0, 0x01a600000063, 0x01ad00001957, 0x01a7000000a2}},
                                            {14, "113496118313637384",                                                       {0x01c30000000e, 0x01d000009897, 0x01d5001b91ed, 0x01c7000000d3}},
                                            {15, "1041048764674763992",                                                      {0x01e500000039, 0x01e700000021, 0x01e600000057, 0x01e700000008, 0x01f6002a0507}},
                                            {16, "9612466889055151536",                                                      {0x0208000000ad, 0x02060000005e, 0x020e00007f66, 0x0209000000bb, 0x020b00000c86}},
                                            {17, "136737887682226879624",                                                    {0x02280000010b, 0x02360037680e, 0x022a0000032b, 0x02230000000c, 0x02260000000d}},
                                            {18, "1206528160113557722640",                                                   {0x025a027e4a09, 0x025b0686e653}},
                                            {19, "13392816195309162358064",                                                  {0x02730002b636, 0x027100022338, 0x02710001844f}},
                                            {20, "108531085367714715815056",                                                 {0x028a00000014, 0x029700157282, 0x028e0000179d, 0x028600000041}},
                                            {21, "2540872174610566355502848",                                                {0x02bc1c8cab69, 0x02bc1b171ccd}},
                                            {22, "25205610729719364390501512",                                               {0x02ce0000514f, 0x02de4a5cd19d, 0x02c40000001f, 0x02c7000000e5}},
                                            {23, "264110507278113870447865504",                                              {0x02ffaa02a145, 0x02fb0b88bc3e}},
                                            {24, "3210476261435427594207226296",                                             {0x031c0d1880f7, 0x031f726ce58d}},
                                            {25, "36265131233527068793914410112",                                            {0x032f00001a37, 0x033700496e85, 0x032a000004c0, 0x032a00000589}},
                                            {26, "482147359703158945661493492672",                                           {0x035e342d290d, 0x035fc74afbab}},
                                            {27, "4704079544225025186901392716640",                                          {0x03710002a137, 0x036f00009f83, 0x037d20b0c8ca}},
                                            {28, "55028898369918937263793273057074",                                         {0x038e000053d8, 0x039a063b8c81, 0x0396001d17e7}},
                                            {29, "631003065226222483111140553534044",                                        {0x03a40000001d, 0x03b5003ddc16, 0x03a9000000cf, 0x03ab000004e2, 0x03b00001ebe8}},
                                            {30, "8339455361149178432323464443273928",                                       {0x03d6006e31d4, 0x03c400000007, 0x03d4001de210, 0x03d10000701c}},
                                            {31, "93110954496457961088201456094928624",                                      {0x03e8000000c6, 0x03fc0e202d16, 0x03ed0000343c, 0x03e40000000c, 0x03ea0000060b}},
                                            {32, "1094942171883116264584750003303308336",                                    {0x0413000a7715, 0x040600000035, 0x04040000000c, 0x0416004dba6f, 0x040d00000c30}},
                                            {33, "11088514349430703105880700615468197880",                                   {0x042f000092bf, 0x0433000dcf30, 0x043b0ba6a833, 0x042500000033}},
                                            {34, "64879528733691864228062104929444374160",                                   {0x04530007e4d0, 0x045200004ef6, 0x045f0a86793c}},
                                            {35, "1475846283891649565543108132745966356352",                                 {0x046400000014, 0x04790249409c, 0x047901d001b6, 0x0469000002dd, 0x046400000013}},
                                            {36, "19413901561926306325428147820550396046520",                                {0x04900001b06d, 0x048f0000d3c8, 0x048c00001aa5, 0x048d0000163c, 0x048c00001a09}},
                                            {37, "159055763910156994330370636154154140991120",                               {0x04a8000000f7, 0x04ae00005e32, 0x04b8004c7939, 0x04a40000000f, 0x04b30003c28d}},
                                            {38, "2571479071919307024156660155287117410397792",                              {0x04de10271242, 0x04d400183722, 0x04d600358f23}},
                                            {39, "26279057335565697597817304245652661439331616",                             {0x04f800a5cd5f, 0x04f20000a473, 0x04fffda424f4}},
                                            {40, "350449250299191552757017197383363115006044240",                            {0x05120002a66e, 0x051c19db640c, 0x051c027b6de8}},
                                            {41, "4121492884951651168965118993549676582542400464",                           {0x053b0ac79e5b, 0x052e000056b9, 0x052f00000448, 0x05320005f225}},
                                            {42, "47422561793075568064338079744175942965044883712",                          {0x055e6ecfa0f9, 0x055500150755, 0x055901d61e7b}},
                                            {43, "436307855872089341868311742005996218703462005536",                         {0x056c00001543, 0x05700001e180, 0x05760040053b, 0x057a05c3f7e7}},
                                            {44, "3804360722211853336958218042885649360855323944928",                        {0x0590000060ae, 0x058b00000310, 0x058b00000e8a, 0x059a02dcc23a, 0x058c0000184c}},
                                            {45, "65151608045100880885757631319298377043157259136520",                       {0x05b600385eef, 0x05aa00000278, 0x05b500317b3e, 0x05b9018e4fa4}},
                                            {46, "863340511301953589894631656146701288891132353068928",                      {0x05d400128b7c, 0x05d50024a086, 0x05d40001e442, 0x05d200001aa3}},
                                            {47, "10120911250746156777647882621327679717108323331331152",                    {0x05e7000000bd, 0x05f700289f49, 0x05f50037601e, 0x05f000019fe4, 0x05ec00000a7d}},
                                            {48, "97041468398499121742697738453349796312081318053362264",                    {0x061e55bb74bf, 0x06160011a50c, 0x061e2ccca8ee}},
                                            {49, "974305863089717877811557447179833284460588578066241200",                   {0x062f00007fad, 0x06310003d1a2, 0x0636002e1152, 0x062b00000c1e, 0x06300000f1f2}},
                                            {50, "13820218507130517959977990899233689607269999740830271984",                 {0x0653000879a9, 0x06550010cb3e, 0x065901c95bea, 0x06520001ca4f}},
                                            {51, "164933735747886426625644943003786984289280865990937384352",                {0x067e56f7e31e, 0x067d30ccdace, 0x067a03b97127}},
                                            {52, "1522212771412914239170769099971030364313763097600382187632",               {0x069b031f4b9a, 0x069600399b41, 0x069500053544, 0x068f0000e506}},
                                            {53, "19769966349012514466658387926172026731841350620329667095008",              {0x06b902ab06c6, 0x06b800098e74, 0x06ac00001d16, 0x06b900e0fec7}},
                                            {54, "249121027098300229959724072287782800940720820069753385890320",             {0x06d40017f684, 0x06db077382e9, 0x06dd2ab2e960, 0x06c300000000, 0x06c7000000eb}},
                                            {55, "2266620498320426291445385902222959897555229607085194487052416",            {0x06e400000017, 0x06ff15dd9903, 0x06f900f4e94f, 0x06f1000020b0, 0x06ea000003f8}},
                                            {56, "35959294699197621547358763195319341887736472137405760932207536",           {0x071d2f8fe0a1, 0x071e124dabef, 0x071f6b8342ab}},
                                            {57, "398740357361279721644581424331615153386562121551817177623063392",          {0x073700609bbd, 0x0732000129c3, 0x073300053eec, 0x072500000025, 0x0738011f538d}},
                                            {58, "2735133064294812712065580442536095846662253756945860704583868032",         {0x07470000009c, 0x0752000632c6, 0x074c000000f1, 0x07560076dd81, 0x075fbff881a0}},
                                            {59, "56538535389889527090877058115887708406482980719397981738168719488",        {0x077e6f250266, 0x07770080a42d, 0x07770057e926, 0x07700001680d}},
                                            {60, "624300455589460716517571889021716155036534686826286991347147329634",       {0x078b00000c74, 0x078700000059, 0x078e000077a9, 0x079f96e62dd5, 0x079d0a7c7154}},
                                            {61, "7548126742950807534514428958297415809638067982382204107783388826916",      {0x07ad00003cc9, 0x07be5a808005, 0x07b8017f907f, 0x07bb04883670}},
                                            {62, "94025673319571255445601240506498724495871554488991581786984718721220",     {0x07dc0f7449f5, 0x07ce00007898, 0x07cf0000a8c7, 0x07d300056f17, 0x07d20007c9da}},
                                            {63, "1034815408916808707460852772817964277108646195129215789731967957926664",   {0x07f800a4be47, 0x07f801ad7d40, 0x07f500233cfb, 0x07fb0d037e18}},
                                            {64, "11665674670632911442850578988937965970298641437937663379320696091286228",  {0x081500227243, 0x081300015d13, 0x08120007a00c, 0x081100000e72, 0x081500161308}},
                                            {65, "151004895377082040807719116839380646994917227726287809466976107672940648", {0x083d14c1eb0c, 0x08340002adde, 0x083901f0e9bf, 0x08380104c908}}};

//=================================================================================================
CExampleReceiver::CExampleReceiver(std::initializer_list<uint64_t> data)
        : m_Data(data){}

//-------------------------------------------------------------------------------------------------
bool CExampleReceiver::Recv(uint64_t & fragment){
    if (!m_Data.size())
        return false;
    this_thread::sleep_for(std::chrono::milliseconds(rand() % 100));
    fragment = m_Data.front();
    m_Data.pop_front();
    return true;
}

//=================================================================================================
void FragmentSender(function<void(uint64_t)> target, std::initializer_list<uint64_t> data){
    for (auto x : data){
        target(x);
        this_thread::sleep_for(std::chrono::milliseconds(rand() % 100));
    }
}

//=================================================================================================
void CExampleTransmitter::Send(uint32_t id, const CBigInt & cnt){
    bool found = false, match = false;

    for (const auto & x : g_TestSets)
        if (x.m_ID == id){
            found = true;
            match = cnt.CompareTo(CBigInt(x.m_Result)) == 0;
            break;
        }

    printf("Finished msg %u, cnt=%s\n", id, cnt.ToString().c_str());
    printf("\tfound=%s, match = %s\n", found ? "true" : "false", match ? "true" : "false");
    m_Sent++;
    this_thread::sleep_for(std::chrono::milliseconds(rand() % 100));
}

//-------------------------------------------------------------------------------------------------
void CExampleTransmitter::Incomplete(uint32_t id){
    printf("Incomplete msg %u\n", id);
    m_Incomplete++;
    this_thread::sleep_for(std::chrono::milliseconds(rand() % 100));
}

//-------------------------------------------------------------------------------------------------
size_t CExampleTransmitter::TotalSent(void) const{
    return m_Sent;
}

//-------------------------------------------------------------------------------------------------
size_t CExampleTransmitter::TotalIncomplete(void) const{
    return m_Incomplete;
}
